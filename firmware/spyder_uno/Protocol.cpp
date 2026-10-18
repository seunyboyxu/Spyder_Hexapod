#include "Protocol.h"
#include <stdio.h>
#include <string.h>
namespace spyder {
static const char *errorName(Result r) {
  switch (r) {
  case Result::Ok:
    return "OK";
  case Result::Invalid:
    return "INVALID";
  case Result::Range:
    return "RANGE";
  case Result::State:
    return "STATE";
  case Result::Calibration:
    return "CALIBRATION";
  case Result::Pose:
    return "POSE";
  case Result::Overflow:
    return "OVERFLOW";
  default:
    return "STORAGE";
  }
}
void Protocol::reply(uint16_t id, Result r) {
  char line[32];
  if (r == Result::Ok)
    snprintf(line, sizeof line, "ACK %u\n", id);
  else
    snprintf(line, sizeof line, "ERR %u %s\n", id, errorName(r));
  sink_.write(line);
}
void Protocol::feed(char ch, uint32_t now) {
  if (ch == '\n') {
    buffer_[length_] = 0;
    if (overflow_)
      reply(0, Result::Overflow);
    else if (bad_)
      reply(0, Result::Invalid);
    else
      execute(now);
    length_ = 0;
    bad_ = overflow_ = sawCR_ = false;
    return;
  }
  if (ch == '\r') {
    if (sawCR_)
      bad_ = true;
    sawCR_ = true;
    return;
  }
  if (sawCR_)
    bad_ = true;
  if (ch < 32 || ch > 126)
    bad_ = true;
  if (length_ >= sizeof buffer_ - 1) {
    overflow_ = true;
    return;
  }
  buffer_[length_++] = ch;
}
static bool number(const char *s, int32_t &n) {
  if (!s || !*s)
    return false;
  bool negative = *s == '-';
  if (negative)
    ++s;
  if (!*s)
    return false;
  uint32_t value = 0;
  for (; *s; ++s) {
    if (*s < '0' || *s > '9')
      return false;
    value = value * 10 + (*s - '0');
    if (value > 65535)
      return false;
  }
  n = negative ? -int32_t(value) : int32_t(value);
  return true;
}
void Protocol::status(uint16_t id) {
  char line[112];
  snprintf(line, sizeof line, "BEGIN %u %u %u\n", id, c_.stopped(),
           c_.config().standingValid);
  sink_.write(line);
  for (uint8_t i = 0; i < JointCount; ++i) {
    const JointConfig &j = c_.config().joints[i];
    const JointState &s = c_.joint(i);
    snprintf(line, sizeof line, "JOINT %u %u %u %u %u %u %d %u %u %u %u %u\n",
             id, i, Controller::pin(i), j.minimum, j.maximum, j.initial,
             j.direction, j.calibrated, s.attached, s.commanded, s.target,
             c_.config().standing[i]);
    sink_.write(line);
  }
  snprintf(line, sizeof line, "END %u\n", id);
  sink_.write(line);
}
void Protocol::execute(uint32_t now) {
  char *tokens[16];
  uint8_t count = 0;
  char *save = nullptr;
  for (char *t = strtok_r(buffer_, " ", &save); t;
       t = strtok_r(nullptr, " ", &save)) {
    if (count == 16) {
      reply(0, Result::Invalid);
      return;
    }
    tokens[count++] = t;
  }
  int32_t id = 0;
  if (count < 2 || !number(tokens[0], id) || id < 1) {
    reply(0, Result::Invalid);
    return;
  }
  const char *verb = tokens[1];
  uint8_t argc = count - 2;
  int32_t args[12];
  if (argc > 12) {
    reply(id, Result::Invalid);
    return;
  }
  for (uint8_t i = 0; i < argc; ++i)
    if (!number(tokens[i + 2], args[i])) {
      reply(id, Result::Invalid);
      return;
    }
  Result r = Result::Invalid;
  if (!strcmp(verb, "HELLO") && argc == 0) {
    sink_.write("SPYDER 2\n");
    r = Result::Ok;
  } else if (!strcmp(verb, "STATUS") && argc == 0) {
    status(id);
    r = Result::Ok;
  } else if (!strcmp(verb, "PING") && argc == 0) {
    c_.heartbeat(now);
    r = Result::Ok;
  } else if (!strcmp(verb, "STOP") && argc == 0) {
    c_.stop();
    r = Result::Ok;
  } else if (!strcmp(verb, "RELEASE") && argc == 0) {
    c_.release();
    r = Result::Ok;
  } else if (!strcmp(verb, "RESUME") && argc == 0)
    r = c_.resume(now);
  else if (!strcmp(verb, "ARM") && argc == 2) {
    if (args[0] >= 0 && args[0] < 12 && args[1] >= 0)
      r = c_.arm(args[0], args[1], now);
  } else if (!strcmp(verb, "MOVE") && argc == 3) {
    if (args[0] >= 0 && args[0] < 12 && args[1] >= 0 && args[2] >= 0)
      r = c_.move(args[0], args[1], args[2]);
  } else if (!strcmp(verb, "CONFIG") && argc == 6) {
    if (args[0] >= 0 && args[0] < 12 && args[1] >= 0 && args[2] >= 0 &&
        args[3] >= 0 && (args[4] == -1 || args[4] == 1) &&
        (args[5] == 0 || args[5] == 1)) {
      JointConfig j = {uint16_t(args[1]), uint16_t(args[2]), uint16_t(args[3]),
                       int8_t(args[4]), args[5] == 1};
      r = c_.configure(args[0], j);
    }
  } else if (!strcmp(verb, "CAPTURE") && argc == 0)
    r = c_.captureStanding();
  else if (!strcmp(verb, "STAND") && argc == 1 && args[0] >= 0)
    r = c_.stand(args[0]);
  else if (!strcmp(verb, "POSE") && argc == 12) {
    uint16_t pose[12];
    bool valid = true;
    for (uint8_t i = 0; i < 12; ++i) {
      if (args[i] < 0)
        valid = false;
      pose[i] = args[i];
    }
    if (valid)
      r = c_.setStanding(pose);
  } else if (!strcmp(verb, "SAVE") && argc == 0)
    r = c_.allDetached() ? settings_.save() : Result::State;
  else if (!strcmp(verb, "LOAD") && argc == 0)
    r = c_.allDetached() ? settings_.load() : Result::State;
  reply(id, r);
}
} // namespace spyder
