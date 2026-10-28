#include "CommandInputs.h"
#include <string.h>
namespace legtest {
CommandInputs::CommandInputs(InputSafety &s): input_{}, safety_(s), generation_(0) {}
void CommandInputs::cancel(uint8_t source, bool release, bool discardCurrent) {
  for (uint8_t i=0;i<2;++i) {
    Input &in=input_[i];
    in.discard = in.discard || (in.used && !in.complete);
    in.used=0; in.complete=false;
  }
  if (discardCurrent) input_[source].discard=true;
  ++generation_;
  if (release) safety_.release(); else safety_.hold();
}
void CommandInputs::cancelAll(bool release) { cancel(0,release,false); }
void CommandInputs::resetSource(uint8_t source) { if(source<2) input_[source]=Input{}; }
void CommandInputs::feed(uint8_t source, char c, uint32_t now) {
  if (source>=2) return;
  Input &in=input_[source];
  if (c=='!') {
    cancel(source,true,true); in.notices|=Released; return;
  }
  if (c=='\r' || c=='\n') {
    if (in.discard) { in.discard=false; in.used=0; return; }
    if (in.complete || !in.used) return;
    in.text[in.used]=0;
    uint8_t start=0;
    while(in.text[start]==' ' || in.text[start]=='\t') ++start;
    if(start) { in.used-=start; memmove(in.text,in.text+start,in.used+1); }
    while(in.used && (in.text[in.used-1]==' ' || in.text[in.used-1]=='\t'))
      in.text[--in.used]=0;
    if (!in.used) return;
    const bool off=!strcmp(in.text,"off"), stop=!strcmp(in.text,"stop");
    if (off || stop) {
      in.used=0;
      cancel(source,off,false); in.notices|=off ? Released : Held;
    } else in.complete=true;
    return;
  }
  if(in.discard) return;
  if(in.complete || (c<32 && c!='\t') || c>126 || in.used==LineSize-1) {
    cancel(source,false,true); in.notices|=BadLine; return;
  }
  in.text[in.used++]=c; in.lastByte=now;
}
void CommandInputs::expire(uint32_t now) {
  for (uint8_t i=0;i<2;++i) {
    Input &in=input_[i];
    if (in.used && !in.complete && uint32_t(now-in.lastByte)>2000) {
      cancel(i,false,true); in.notices|=Expired;
    }
  }
}
bool CommandInputs::take(uint8_t source, char (&line)[LineSize]) {
  if(source>=2 || !input_[source].complete) return false;
  Input &in=input_[source];
  memcpy(line,in.text,in.used+1); in.used=0; in.complete=false; return true;
}
uint8_t CommandInputs::notice(uint8_t source) {
  if(source>=2) return 0;
  const uint8_t result=input_[source].notices; input_[source].notices=0; return result;
}
} // namespace legtest
