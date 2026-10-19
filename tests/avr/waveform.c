#include <math.h>
#include <simavr/avr_ioport.h>
#include <simavr/avr_uart.h>
#include <simavr/sim_avr.h>
#include <simavr/sim_elf.h>
#include <simavr/sim_hex.h>
#include <simavr/sim_io.h>
#include <simavr/sim_irq.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
static avr_t *mcu;
static avr_irq_t *input;
static char phase[40] = "startup";
static FILE *trace;
static char uart[200000];
static size_t used = 0;
typedef struct {
  int pin, level, count;
  double rise, previous, widthMin, widthMax, periodMin, periodMax;
} Pin;
static Pin pins[12];
static double nowus() { return mcu->cycle / 16.0; }
static void edge(struct avr_irq_t *irq, uint32_t value, void *param) {
  (void)irq;
  Pin *p = param;
  int high = value & 1;
  if (high == p->level)
    return;
  p->level = high;
  double now = nowus();
  fprintf(trace, "%s,%.4f,%d,%d\n", phase, now, p->pin, high);
  if (high) {
    if (p->count > 0) {
      double d = now - p->previous;
      if (d < p->periodMin)
        p->periodMin = d;
      if (d > p->periodMax)
        p->periodMax = d;
    }
    p->previous = p->rise = now;
  } else if (p->rise > 0) {
    double width = now - p->rise;
    if (width < p->widthMin)
      p->widthMin = width;
    if (width > p->widthMax)
      p->widthMax = width;
    p->count++;
  }
}
static void output(struct avr_irq_t *irq, uint32_t value, void *param) {
  (void)irq;
  (void)param;
  if (used < sizeof(uart) - 1)
    uart[used++] = value;
  uart[used] = 0;
}
static void runus(uint64_t duration) {
  uint64_t target = mcu->cycle + duration * 16;
  while (mcu->cycle < target) {
    int state = avr_run(mcu);
    if (state == cpu_Done || state == cpu_Crashed) {
      fprintf(stderr, "MCU stopped state=%d pc=%u\n", state, mcu->pc);
      exit(3);
    }
  }
}
static void sendraw(const char *s) {
  for (; *s; s++) {
    avr_raise_irq(input, *s);
    runus(120);
  }
}
static void send(const char *s) {
  sendraw(s);
  runus(5000);
}
static void resetstats() {
  for (int i = 0; i < 12; i++) {
    pins[i].count = 0;
    pins[i].rise = 0;
    pins[i].previous = 0;
    pins[i].widthMin = pins[i].periodMin = 1e12;
    pins[i].widthMax = pins[i].periodMax = 0;
  }
}
static void printstats() {
  for (int i = 0; i < 12; i++)
    printf("D%d: pulses=%d width_us=%.3f..%.3f period_us=%.3f..%.3f "
           "final_level=%d\n",
           pins[i].pin, pins[i].count, pins[i].widthMin, pins[i].widthMax,
           pins[i].periodMin, pins[i].periodMax, pins[i].level);
}
int main(int argc, char **argv) {
  if (argc != 4 && argc != 6) {
    fprintf(stderr, "Usage: harness elf mode trace.csv [benchMin benchMax]\n");
    return 2;
  }
  elf_firmware_t firmware = {0};
  if (elf_read_firmware(argv[1], &firmware))
    return 2;
  mcu = avr_make_mcu_by_name("atmega328p");
  avr_init(mcu);
  mcu->frequency = 16000000;
  mcu->vcc = mcu->avcc = mcu->aref = 5000;
  avr_load_firmware(mcu, &firmware);
  mcu->log = 0;
  trace = fopen(argv[3], "w");
  fprintf(trace, "phase,time_us,pin,level\n");
  for (int i = 0; i < 12; i++) {
    pins[i].pin = i + 2;
    char port = i < 6 ? 'D' : 'B';
    int bit = i < 6 ? i + 2 : i - 6;
    avr_irq_register_notify(
        avr_io_getirq(mcu, AVR_IOCTL_IOPORT_GETIRQ(port), bit), edge, &pins[i]);
  }
  resetstats();
  input = avr_io_getirq(mcu, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_INPUT);
  avr_irq_register_notify(
      avr_io_getirq(mcu, AVR_IOCTL_UART_GETIRQ('0'), UART_IRQ_OUTPUT), output,
      NULL);
  uint32_t flags = 0;
  avr_ioctl(mcu, AVR_IOCTL_UART_SET_FLAGS('0'), &flags);
  runus(200000);
  printf("startup pulses=%d, UART=%s", pins[0].count, uart);
  send("1 RESUME\n");
  send("2 ARM 0 1500\n");
  runus(100000);
  if (!strcmp(argv[2], "bench-sweep")) {
    const int minimum = argc == 6 ? atoi(argv[4]) : 1000;
    const int maximum = argc == 6 ? atoi(argv[5]) : 2000;
    if (minimum >= 1500 || maximum <= 1500 || minimum < 1 || maximum > 5000)
      return 2;
    send("50 RELEASE\n");
    strcpy(phase, "bench-center");
    send("60 CENTER 4\n");
    runus(100000);
    strcpy(phase, "bench-sweep");
    char cmd[80];
    snprintf(cmd, sizeof cmd, "61 SWEEP 4 %d %d 100\n", minimum, maximum);
    send(cmd);
    const double finish = nowus() + 2000000.0 + (maximum - minimum) * 20000.0;
    for (int i = 0; nowus() < finish; i++) {
      snprintf(cmd, sizeof cmd, "%d PING\n", 1000 + i);
      send(cmd);
      runus(200000);
    }
    send("62 STATUS\n");
    runus(100000);
    strcpy(phase, "bench-release");
    send("63 RELEASE\n");
    runus(100000);
    printstats();
  } else if (!strcmp(argv[2], "steady") ||
             !strncmp(argv[2], "release-offset-", 15)) {
    for (int i = 1; i < 12; i++) {
      char cmd[80];
      snprintf(cmd, sizeof cmd, "%d ARM %d 1500\n", i + 2, i);
      send(cmd);
    }
    strcpy(phase, "steady");
    resetstats();
    if (!strncmp(argv[2], "release-offset-", 15)) {
      runus(100000 + atoi(argv[2] + 15));
      strcpy(phase, "release");
      send("50 RELEASE\n");
      runus(100000);
    } else
      runus(500000);
    printstats();
  } else if (!strcmp(argv[2], "release-high")) {
    sendraw("50 RELEASE");
    while (!pins[0].level)
      runus(1);
    double highat = nowus();
    strcpy(phase, "release");
    sendraw("\n");
    runus(100000);
    printf("Release started while D2 HIGH at %.3fus; 100ms later D2 level=%d, "
           "elapsed HIGH=%.3fus\n",
           highat, pins[0].level, nowus() - pins[0].rise);
    printstats();
  } else if (!strcmp(argv[2], "reattach")) {
    sendraw("50 RELEASE");
    while (!pins[0].level)
      runus(1);
    send("\n");
    runus(40000);
    strcpy(phase, "reattach");
    resetstats();
    send("51 ARM 0 1500\n");
    runus(200000);
    printstats();
  } else if (!strcmp(argv[2], "serial")) {
    strcpy(phase, "serial");
    resetstats();
    send("5 MOVE 0 1600 50\n");
    for (int i = 0; i < 15; i++) {
      char cmd[64];
      snprintf(cmd, sizeof cmd, "%d PING\n", 100 + i * 2);
      send(cmd);
      snprintf(cmd, sizeof cmd, "%d STATUS\n", 101 + i * 2);
      sendraw(cmd);
      runus(70000);
    }
    printstats();
  }
  printf("\nUART tail:\n%s\n", used > 3000 ? uart + used - 3000 : uart);
  fclose(trace);
  return 0;
}
