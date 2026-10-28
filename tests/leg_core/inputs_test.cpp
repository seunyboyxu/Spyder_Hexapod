#include "CommandInputs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
using namespace legtest;
struct Safety : InputSafety {
  int holds=0, releases=0;
  void hold() override { ++holds; }
  void release() override { ++releases; }
};
static void feed(CommandInputs &in,uint8_t source,const char *text,uint32_t now=0) {
  for(;*text;++text) in.feed(source,*text,now);
}
int main() {
  Safety safety; CommandInputs in(safety); char line[CommandInputs::LineSize];
  feed(in,0,"cal "); feed(in,1,"show\n"); feed(in,0,"0\r\n");
  assert(in.take(1,line) && !strcmp(line,"show"));
  assert(in.take(0,line) && !strcmp(line,"cal 0"));
  assert(!in.take(0,line));
  // A STOP/release must cancel old queued/partial motion on BOTH inputs.
  feed(in,0,"cal 0"); feed(in,1," !");
  assert(safety.releases==1 && in.generation()==1);
  feed(in,0,"\n"); feed(in,1,"cal 1\n");
  assert(!in.take(0,line) && !in.take(1,line));
  assert(in.notice(1)&CommandInputs::Released);
  feed(in,0,"cal 0\n"); feed(in,1," stop \n");
  assert(safety.holds==1 && !in.take(0,line));
  assert(in.notice(1)&CommandInputs::Held);
  feed(in,1,"off\n"); assert(safety.releases==2);
  // Queue saturation rejects old/new lines and holds; no truncated command.
  feed(in,0,"cal 0\ncal 1\n"); assert(!in.take(0,line));
  assert(in.notice(0)&CommandInputs::BadLine);
  for(int i=0;i<80;++i) in.feed(1,' ',0);
  feed(in,1,"cal 0\n"); assert(!in.take(1,line));
  assert(in.notice(1)&CommandInputs::BadLine);
  feed(in,1,"cal 0",10); in.expire(2011); feed(in,1,"\n",2012);
  assert(!in.take(1,line) && (in.notice(1)&CommandInputs::Expired));
  feed(in,1,"show\r\n",2020); assert(in.take(1,line) && !strcmp(line,"show"));
  feed(in,0,"help",0xfffffff0U); in.expire(1990);
  assert(in.notice(0)&CommandInputs::Expired);
  puts("dual-input isolation, urgent safety cancellation, strict lines, queue bounds and expiry passed");
}
