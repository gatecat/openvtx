#ifndef MIWI2_INPUT_HPP
#define MIWI2_INPUT_HPP

#include "SDL2/SDL.h"
#include <cstdint>
using namespace std;

namespace VTxx {
class MiWi2Input {
public:
  void write(uint8_t addr, uint8_t data);
  uint8_t read(uint8_t addr);
  void process_event(SDL_Event *ev);
  void notify_vblank();

  uint16_t btn_state = 0;

private:
  bool is_vblank = false;
  int read_idx = 0;
};
}; // namespace VTxx

#endif /* end of include guard: MIWI2_INPUT_HPP */
