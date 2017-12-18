#include "mmu.hpp"
#include "ppu.hpp"
#include "util.hpp"
#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>
using namespace std;

namespace VTxx {

uint8_t control_reg[256] = {0};
uint8_t cpu_ram[8192];

static uint8_t rom[32 * 1024 * 1024];

ReadHandler reg_read_fn[256] = {nullptr};
WriteHandler reg_write_fn[256] = {nullptr};

void mmu_init() {
  // TODO: default paging values?
}

void load_rom(const string &filename) {
  ifstream romf(filename);
  if (!romf) {
    cerr << "Failed to load ROM" << endl;
    assert(false);
  }
  romf.read(reinterpret_cast<char *>(rom), sizeof(rom));
  size_t romsize = romf.gcount();
  cout << "Loaded ROM, size = " << (romsize / 1024) << "KB" << endl;
}

const int reg_prg_bank1_reg3 = 0x00;
const int reg_prg_bank0_reg0 = 0x07;
const int reg_prg_bank0_reg1 = 0x08;
const int reg_prg_bank0_reg2 = 0x09;
const int reg_prg_bank0_reg3 = 0x0A;
const int reg_prg_bank0_sel = 0x0B;
const int reg_prg_bank1_reg2 = 0x0C;
const int reg_prg_bank1_reg0 = 0x10;
const int reg_prg_bank1_reg1 = 0x11;
const int reg_prg_bank0_reg4 = 0x12;
const int reg_prg_bank0_reg5 = 0x13;

const int reg_prg_bank1_reg0_rd = 0x12;
const int reg_prg_bank1_reg1_rd = 0x13;
const int reg_prg_bank0_reg4_rd = 0x10;
const int reg_prg_bank0_reg5_rd = 0x11;

const int reg_prg_bank1_reg4_5 = 0x18;

inline uint32_t decode_address(uint16_t addr) {
  if (addr < 0x4000)
    return addr;
  uint8_t tp = 0;
  bool comr6 = get_bit(control_reg[0x05], 6);
  bool pq2en = get_bit(control_reg[0x0B], 6);
  bool ext2421 = get_bit(control_reg[0x1C], 5);
  uint32_t pa = addr & 0x1FFF;
  switch ((pq2en << 4) | (comr6 << 3) | ((addr >> 13) & 0x07)) {
  case 0b00010:
  case 0b01010:
  case 0b10010:
  case 0b11010:
    tp = control_reg[reg_prg_bank0_reg4];
    break;
  case 0b00011:
  case 0b01011:
  case 0b10011:
  case 0b11011:
    tp = control_reg[reg_prg_bank0_reg5];
    break;
  case 0b00100:
  case 0b01110:
  case 0b10100:
  case 0b11110:
    tp = control_reg[reg_prg_bank0_reg0];
    break;
  case 0b00101:
  case 0b01101:
  case 0b10101:
  case 0b11101:
    tp = control_reg[reg_prg_bank0_reg1];
    break;
  case 0b00110:
  case 0b01100:
    tp = 0xFE;
    break;
  case 0b00111:
  case 0b01111:
  case 0b10111:
  case 0b11111:
    tp = 0xFF;
    break;
  case 0b10110:
  case 0b11100:
    tp = control_reg[reg_prg_bank0_reg2];
    break;
  default:
    assert(false);
  }
  uint8_t pq3 = control_reg[reg_prg_bank0_reg3];
  uint8_t pa20_13 = 0;
  int sel = control_reg[reg_prg_bank0_sel] & 0x07;
  if (sel == 0x07) {
    pa20_13 = tp;
  } else {
    uint8_t mask = (1 << (6 - sel)) - 1;
    pa20_13 = (pq3 & ~mask) | (tp & mask);
  }
  pa |= (pa20_13 << 13);
  uint8_t pa24_21 = 0;
  if (ext2421 && get_bit(addr, 15)) {
    pa24_21 = control_reg[reg_prg_bank1_reg3] & 0x0F;
  } else {
    switch ((pq2en << 4) | (comr6 << 3) | ((addr >> 13) & 0x07)) {
    case 0b00010:
    case 0b01010:
    case 0b10010:
    case 0b11010:
      pa24_21 = control_reg[reg_prg_bank1_reg4_5] & 0x0F;
      break;
    case 0b00011:
    case 0b01011:
    case 0b10011:
    case 0b11011:
      pa24_21 = (control_reg[reg_prg_bank1_reg4_5] >> 4) & 0x0F;
      break;
    case 0b00100:
    case 0b01110:
    case 0b10100:
    case 0b11110:
      pa24_21 = control_reg[reg_prg_bank1_reg0] & 0x0F;
      break;
    case 0b00101:
    case 0b01101:
    case 0b10101:
    case 0b11101:
      pa24_21 = control_reg[reg_prg_bank1_reg1] & 0x0F;
      break;
    case 0b10110:
    case 0b11100:
      pa24_21 = control_reg[reg_prg_bank1_reg2] & 0x0F;
      break;
    case 0b00110:
    case 0b00111:
    case 0b01100:
    case 0b01111:
    case 0b10111:
    case 0b11111:
      pa24_21 = control_reg[reg_prg_bank1_reg3] & 0x0F;
      break;
    default:
      assert(false);
    }
  }
  pa |= ((pa24_21 & 0x0F) << 21);
  return pa;
}

uint8_t read_mem_virtual(uint16_t addr) {
  if (addr < 0x2000) {
    return cpu_ram[addr];
  } else if (addr >= 0x4000) {
    return rom[decode_address(addr)];
  } else if (addr >= 0x2000 && addr <= 0x20FF) {
    return ppu_read(addr & 0xFF);
  } else if (addr >= 0x2100 && addr <= 0x21FF) {
    if ((addr >= 0x210D) && (addr <= 0x210F))
      cout << "IOx READ 0x" << hex << addr << endl;
    // System regs read
    uint8_t reg_addr = addr & 0xFF;
    if (reg_addr == reg_prg_bank0_reg4_rd)
      return control_reg[reg_prg_bank0_reg4];
    else if (reg_addr == reg_prg_bank0_reg5_rd)
      return control_reg[reg_prg_bank0_reg5];
    else if (reg_addr == reg_prg_bank1_reg0_rd)
      return control_reg[reg_prg_bank1_reg0];
    else if (reg_addr == reg_prg_bank1_reg1_rd)
      return control_reg[reg_prg_bank1_reg1];
    else if (reg_read_fn[reg_addr] != nullptr)
      return (reg_read_fn[reg_addr])(addr);
    else
      return control_reg[reg_addr];
  } else {
    // Unmapped space
    assert(false);
  }
}

void write_mem_virtual(uint16_t addr, uint8_t data) {
  if (addr < 0x2000) {
    cpu_ram[addr] = data;
  } else if (addr >= 0x4000) {
    rom[decode_address(addr)] =
        data; // Seems odd but "ROM" might actually be extram
  } else if (addr >= 0x2000 && addr <= 0x20FF) {
    ppu_write(addr & 0xFF, data);
  } else if (addr >= 0x2100 && addr <= 0x21FF) {
    if ((addr >= 0x210D) && (addr <= 0x210F))
      cout << "IOx WRITE " << addr << " d " << int(data) << endl;
    uint8_t reg_addr = addr & 0xFF;
    if (reg_write_fn[reg_addr] != nullptr)
      (reg_write_fn[reg_addr])(addr, data);
    else
      control_reg[reg_addr] = data;
  } else {
    // Unmapped space
    assert(false);
  }
}

uint8_t read_mem_physical(uint32_t addr) {
  assert(addr < sizeof(rom));
  return rom[addr];
}
void write_mem_physical(uint32_t addr, uint8_t data) {
  assert(addr < sizeof(rom));
  rom[addr] = data;
}

string va_to_str(uint16_t va) {
  ostringstream s;
  s << "0x" << hex << va;
  if (va >= 0x4000)
    s << " ( -> 0x" << hex << decode_address(va) << ")";
  return s.str();
}

} // namespace VTxx
