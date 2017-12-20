#include "miwi2_input.hpp"
#include "SDL2/SDL.h"
#include <cassert>
#include <iostream>
#include <map>
#include <vector>
using namespace std;

namespace VTxx {

void MiWi2Input::write(uint8_t addr, uint8_t data) {}

uint8_t MiWi2Input::read(uint8_t addr) {
  if (addr == 0x01) {
    return (read_idx <= 3) ? 0x00 : 0xFF;
  } else if (addr == 0x00) {
    uint8_t res = ~((btn_state >> (4 * read_idx)) & 0x0F);
    read_idx++;
    return res;
  } else {
    assert(false);
  }
}
// Map keys to input bits
static const map<SDL_Scancode, int> keys = {
    {SDL_SCANCODE_X, 7},      {SDL_SCANCODE_Z, 6},    {SDL_SCANCODE_RSHIFT, 5},
    {SDL_SCANCODE_RETURN, 4}, {SDL_SCANCODE_UP, 3},   {SDL_SCANCODE_DOWN, 2},
    {SDL_SCANCODE_LEFT, 1},   {SDL_SCANCODE_RIGHT, 0}};

void MiWi2Input::process_event(SDL_Event *ev) {
  switch (ev->type) {
  case SDL_KEYDOWN:
    if (keys.find(ev->key.keysym.scancode) != keys.end()) {
      cout << "keydown " << keys.at(ev->key.keysym.scancode) << endl;
      btn_state |= (1 << keys.at(ev->key.keysym.scancode));
    }
    break;
  case SDL_KEYUP:
    if (keys.find(ev->key.keysym.scancode) != keys.end()) {
      cout << "keyup " << keys.at(ev->key.keysym.scancode) << endl;
      btn_state &= ~(1 << keys.at(ev->key.keysym.scancode));
    }
    break;
  }
}

void MiWi2Input::notify_vblank() { read_idx = 0; }

} // namespace VTxx
