#ifndef PPU_H
#define PPU_H
#include <atomic>
#include <cstdint>
#include <thread>
using namespace std;

namespace VTxx {
// Multithreaded VT1682 PPU - at the moment this is a simple but very inaccurate
// implementation

void ppu_init();
void ppu_begin_render();

// Write/Read PPU address space, address is 0..255 relative to 0x2000
void ppu_write(uint8_t addr, uint8_t data);
uint8_t ppu_read(uint8_t addr);

bool is_render_done();

// Return the PPU output as a 256x240 ARGB buffer
uint32_t *get_render_buffer();

} // namespace VTxx

#endif /* end of include guard: PPU_H */
