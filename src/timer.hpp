#ifndef TIMER_H
#define TIMER_H
#include "typedefs.hpp"
#include <cstdint>

namespace VTxx {
enum class TimerType { TIMER_VT_CPU, TIMER_VT_SCPU };

class Timer {
public:
  Timer(TimerType _type, TimerCallback _cb);
  // CPU Timer address is rel to 0x2101
  // SCPU Timer address is rel to 0x2100/0x2110
  void write(uint8_t addr, uint8_t data);
  uint8_t read(uint8_t addr);
  void tick();

private:
  TimerType type;
  TimerCallback cb;
  uint16_t preload = 0;
  uint16_t count = 0;
  uint8_t config = 0;
  bool tsynen = false;
  int tsyn_div = 0;
  bool last_vblank = false;
};

} // namespace VTxx

#endif /* end of include guard: TIMER_H */
