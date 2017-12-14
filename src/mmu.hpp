#ifndef MMU_H
#define MMU_H
#include <stdint>
#include <string>
using namespace std;

// The system control registers, 0x2100 .. 0x21FF
uint8_t control_reg[256];

void mmu_init();
void load_rom(const string &filename);
uint8_t read_mem_virtual(uint16_t addr);
void write_mem_virtual(uint16_t addr, uint8_t data);

uint8_t read_mem_physical(uint32_t addr);
void write_mem_physical(uint32_t addr, uint8_t data);


#endif /* end of include guard: MMU_H */

