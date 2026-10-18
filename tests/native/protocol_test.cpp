#include "Protocol.h"
#include <cassert>
#include <cstdio>
#include <string>
using namespace spyder;
struct O : Outputs {
  void attach(uint8_t, uint8_t, uint16_t) override {}
  void write(uint8_t, uint16_t) override {}
  void detach(uint8_t) override {}
};
struct Sink : TextSink {
  std::string text;
  void write(const char *s) override { text += s; }
};
struct Settings : SettingsService {
  int saves = 0, loads = 0;
  Result save() override {
    ++saves;
    return Result::Ok;
  }
  Result load() override {
    ++loads;
    return Result::Storage;
  }
};
int main() {
  O o;
  Controller c(o);
  c.begin(0);
  c.resume(0);
  Sink sink;
  Settings settings;
  Protocol p(c, sink, settings);
  auto send = [&](std::string s) {
    sink.text.clear();
    for (char ch : s + "\n")
      p.feed(ch, 0);
    return sink.text;
  };
  assert(send("1 HELLO") == "SPYDER 2\nACK 1\n");
  auto status = send("2 STATUS");
  assert(status.find("BEGIN 2 0 0\n") == 0 &&
         status.find("JOINT 2 11 13 ") != std::string::npos &&
         status.find("END 2\nACK 2\n") != std::string::npos);
  for (auto s : {"3 ARM", "3 ARM -1 1500", "3 ARM 12 1500", "3 ARM 0 1500x",
                 "3 ARM 0 9999999999999", "3 CONFIG 0 1400 1600 1500 0 1",
                 "3 CONFIG 0 1400 1600 1500 1 2", "3 STOP extra", "0 STOP",
                 "-1 STOP", "3 WAVE"})
    assert(send(s).find("ERR ") == 0);
  assert(c.allDetached() && !c.stopped());
  assert(send("4 CONFIG 0 1400 1600 1500 -1 1") == "ACK 4\n");
  assert(send("5 ARM 0 1500") == "ACK 5\n");
  assert(send("6 MOVE 0 1510 50") == "ACK 6\n");
  assert(send("7 STOP") == "ACK 7\n" && c.stopped());
  assert(send("8 MOVE 0 1500 50") == "ERR 8 STATE\n");
  assert(send("9 PING") == "ACK 9\n" && c.stopped());
  assert(send("10 RELEASE") == "ACK 10\n" && c.allDetached());
  assert(send("11 RESUME\r") == "ACK 11\n" && !c.stopped());
  assert(send(std::string(200, 'x') + " 12 ARM 0 1500") == "ERR 0 OVERFLOW\n");
  assert(c.allDetached());
  assert(send("13 SAVE") == "ACK 13\n" && settings.saves == 1);
  assert(send("14 LOAD") == "ERR 14 STORAGE\n" && settings.loads == 1);
  assert(send(std::string("15 STOP") + char(0x80)).find("ERR ") == 0 &&
         !c.stopped());
  assert(send(std::string("16 STOP") + char(0) + "ignored").find("ERR ") == 0 &&
         !c.stopped());
  assert(send("16 A\rRM 0 1500").find("ERR ") == 0 && c.allDetached());
  assert(send("17 CAPTURE").find("ERR ") == 0);
  assert(send("18 STAND 50").find("ERR ") == 0);
  for (int i = 0; i < 12; ++i)
    c.configure(i, JointConfig{1400, 1600, 1500, 1, true});
  assert(send("19 POSE 1500 1500 1500 1500 1500 1500 1500 1500 1500 1500 1500 "
              "1700") == "ERR 19 RANGE\n");
  assert(!c.config().standingValid);
  assert(send("20 POSE 1500 1500 1500 1500 1500 1500 1500 1500 1500 1500 1500 "
              "1500") == "ACK 20\n");
  std::puts("protocol: framing, all commands, malformed/bounds/overflow, STOP, "
            "storage dispatch PASS (7 groups)");
}
