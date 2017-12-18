#ifndef VT168_H
#define VT168_H

#include "SDL2/SDL.h"
#include <cstdint>
#include <string>
namespace VTxx {

enum class VT168_Platform { VT168_BASE, VT168_MIWI2 };

void vt168_init(VT168_Platform plat, const std::string &rom);
bool vt168_tick();
void vt168_process_event(SDL_Event *ev);
}; // namespace VTxx

#endif /* end of include guard: VT168_H */
