#pragma once
#include "LegTest.h"
namespace fourleg {
const uint8_t LegCount=4, ChannelCount=12, All=4;
struct Settings { legtest::Settings leg[LegCount]; };
const uint16_t SavedSize=134;
bool encodeSettings(const Settings &settings,uint8_t (&bytes)[SavedSize]);
bool decodeSettings(const uint8_t (&bytes)[SavedSize],Settings &settings);
class Channels {
public:
  virtual ~Channels() {}
  virtual void attach(uint8_t channel,uint16_t pulse)=0;
  virtual void write(uint8_t channel,uint16_t pulse)=0;
  virtual void detach(uint8_t channel)=0;
};
class MappedOutputs : public legtest::Outputs {
  Channels &channels_; uint8_t base_;
public:
  MappedOutputs(Channels &channels,uint8_t base):channels_(channels),base_(base) {}
  void attach(uint8_t joint,uint16_t pulse) override { channels_.attach(base_+joint,pulse); }
  void write(uint8_t joint,uint16_t pulse) override { channels_.write(base_+joint,pulse); }
  void detach(uint8_t joint) override { channels_.detach(base_+joint); }
};
class FourLegTest {
public:
  explicit FourLegTest(Channels &channels);
  void begin(uint32_t now);
  const char *command(char *line,uint32_t now);
  void tick(uint32_t now);
  void stop();
  void release();
  bool anyAttached() const;
  bool busy() const;
  bool arming() const { return arming_; }
  bool coordinated() const { return group_; }
  uint8_t selected() const { return selected_; }
  const legtest::LegTest &leg(uint8_t index) const { return legs_[index]; }
  Settings settings() const;
  bool applySettings(const Settings &settings);
private:
  MappedOutputs maps_[LegCount];
  legtest::LegTest legs_[LegCount];
  uint8_t selected_,armIndex_;
  bool group_,groupStep_,arming_;
  uint32_t lastArm_;
};
}
