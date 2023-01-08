#include "scpu_mem.hpp"
#include "mmu.hpp"
#include <cassert>
#include <iostream>
namespace VTxx {

uint8_t scpu_control_reg[256] = {0};
ReadHandler scpu_reg_read_fn[256] = {nullptr};
WriteHandler scpu_reg_write_fn[256] = {nullptr};

uint8_t scpu_read_mem(uint16_t addr) {
  if ((addr >= 0 && addr < 0x1000) || (addr >= 0x1000 && addr < 0x2000)) {
    uint16_t mem_addr = addr & 0x0FFF;
    return cpu_ram[0x1000 | mem_addr];
  } else if (addr >= 0x2100 && addr < 0x2200) {
    // cout << "scpu read " << addr << endl;
    uint8_t reg_addr = addr & 0xFF;
    if (scpu_reg_read_fn[reg_addr] != nullptr)
      return scpu_reg_read_fn[reg_addr](addr);
    else
      return scpu_control_reg[reg_addr];
  } else {
    assert(false);
  }
}

void scpu_write_mem(uint16_t addr, uint8_t data) {
  if ((addr >= 0 && addr < 0x1000) || (addr >= 0x1000 && addr < 0x2000)) {
    uint16_t mem_addr = addr & 0x0FFF;
    cpu_ram[0x1000 | mem_addr] = data;
  } else if (addr >= 0x2100 && addr < 0x2200) {
    // cout << "scpu write " << addr << " " << int(data) << endl;

    uint8_t reg_addr = addr & 0xFF;
    if (scpu_reg_write_fn[reg_addr] != nullptr)
      scpu_reg_write_fn[reg_addr](addr, data);
    else
      scpu_control_reg[reg_addr] = data;
  } else {
    assert(false);
  }
}
}; // namespace VTxx
