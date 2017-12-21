#include "timer.hpp"
#include "util.hpp"
#include <cassert>
namespace VTxx {

Timer::Timer(TimerType _type, TimerCallback _cb) : type(_type), cb(_cb){};

void Timer::write(uint8_t addr, uint8_t data) {
  if (type == TimerType::TIMER_VT_CPU) {
    switch (addr) {
    case 0x0:
      preload &= 0xFF00;
      preload |= data;
      break;
    case 0x3:
      preload &= 0x00FF;
      preload |= (data << 8UL);
      break;
    case 0x1:
      config = data;
      break;
    case 0x2:
      cb(false);
      break;
    case 0xA:
      tsynen = get_bit(data, 7);
      tsyn_div = 0;
      break;
    default:
      assert(false);
    }
  } else if (type == TimerType::TIMER_VT_SCPU) {
    switch (addr) {
    case 0x0:
      preload &= 0xFF00;
      preload |= data;
      break;
    case 0x1:
      preload &= 0x00FF;
      preload |= (data << 8UL);
      break;
    case 0x2:
      config = data;
      break;
    case 0x3:
      cb(false);
      break;
    default:
      assert(false);
    }
  }
}

uint8_t Timer::read(uint8_t addr) {
  if (type == TimerType::TIMER_VT_CPU) {
    switch (addr) {
    case 0x0:
      return preload & 0xFF;
    case 0x3:
      return (preload >> 8) & 0xFF;
    case 0x1:
      return config;
    case 0xA:
      return tsynen << 7;
    default:
      assert(false);
    }
  } else if (type == TimerType::TIMER_VT_SCPU) {
    switch (addr) {
    case 0x0:
      return preload & 0xFF;
    case 0x1:
      return (preload >> 8) & 0xFF;
    case 0x2:
      return config;
    default:
      assert(false);
    }
  } else {
    assert(false);
  }
}

void Timer::tick() {
  bool do_tick = false;
  if (tsynen) {
    tsyn_div += 1;
    if (tsyn_div >= 340) {
      tsyn_div = 0;
      do_tick = true;
    } else {
      do_tick = false;
    }
  } else {
    do_tick = true;
  }
  if (do_tick && get_bit(config, 0)) {
    if (count == 0xFFFF) {
      if (get_bit(config, 1))
        cb(true);
      count = preload;
    } else {
      count++;
    }
  }
}

} // namespace VTxx
