#ifndef MMU_H
#define MMU_H
#include "typedefs.hpp"
#include <cstdint>
#include <string>
using namespace std;
namespace VTxx {
// The system control registers, 0x2100 .. 0x21FF
extern uint8_t control_reg[256];

// The main 8KB CPU RAM, between 0x0000 and 0x1FFF
extern uint8_t cpu_ram[8192];

void mmu_init();
void load_rom(const string &filename);
uint8_t read_mem_virtual(uint16_t addr);
void write_mem_virtual(uint16_t addr, uint8_t data);

uint8_t read_mem_physical(uint32_t addr);
void write_mem_physical(uint32_t addr, uint8_t data);

// Custom read and write overrides for control registers
// Set to nullptr if just a plain register
extern ReadHandler reg_read_fn[256];
extern WriteHandler reg_write_fn[256];

// These dummy handlers just throw an error and are used
// when a given register doesn't support reading or writing
uint8_t DisallowedReadHandler(uint16_t addr);
void DisallowedWriteHandler(uint16_t addr, uint8_t value);
} // namespace VTxx

#endif /* end of include guard: MMU_H */
