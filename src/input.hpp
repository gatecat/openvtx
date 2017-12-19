#ifndef INPUT_HPP
#define INPUT_HPP

#include "SDL2/SDL.h"
#include <cstdint>
using namespace std;

namespace VTxx {
class InputDev {
public:
  void write(uint8_t addr, uint8_t data);
  uint8_t read(uint8_t addr);
  void process_event(SDL_Event *ev);

  uint8_t btn_state = 0;
  uint8_t shiftreg = 0;
};
}; // namespace VTxx

#endif /* end of include guard: INPUT_HPP */
