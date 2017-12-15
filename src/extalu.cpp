#include "extalu.hpp"
#include <cassert>
namespace VTxx {
ExtALU::ExtALU(bool _rem_quirk, bool _read_offset)
    : rem_quirk(_rem_quirk), read_offset(_read_offset){};

void ExtALU::write(uint8_t addr, uint8_t data) {
  if (addr < 4) {
    operand[addr] = data;
  } else if ((addr >= 4) && (addr < 6)) {
    mul_operand[addr - 4] = data;
    if (addr == 5)
      do_mul();
  } else if ((addr >= 6) && (addr < 8)) {
    div_operand[addr - 6] = data;
    if (addr == 7)
      do_div();
  } else {
    assert(false);
  }
}

uint8_t ExtALU::read(uint8_t addr) {
  uint8_t addr_ofs = addr;
  if (read_offset) {
    assert(addr_ofs >= 8);
    addr_ofs -= 8;
  }
  assert(addr_ofs < 6);
  return result[addr_ofs];
}

void ExtALU::do_mul() {
  uint32_t op1 = (mul_operand[1] << 8UL) | (mul_operand[0]);
  uint32_t op2 = (operand[1] << 8UL) | (operand[0]);
  uint32_t res = op1 * op2;
  result[0] = res & 0xFF;
  result[1] = (res >> 8) & 0xFF;
  result[2] = (res >> 16) & 0xFF;
  result[3] = (res >> 24) & 0xFF;
}

void ExtALU::do_div() {
  uint32_t op1 = (operand[3] << 24UL) | (operand[2] << 16UL) |
                 (operand[1] << 8UL) | (operand[0]);
  uint32_t op2 = (div_operand[1] << 8UL) | (div_operand[0]);

  uint32_t quot = op1 / op2;
  result[0] = quot & 0xFF;
  result[1] = (quot >> 8) & 0xFF;
  result[2] = (quot >> 16) & 0xFF;
  result[3] = (quot >> 24) & 0xFF;
  uint32_t rem = op1 % op2;
  // Is this correct? Does it matter? VT1682 PG is very ambiguous here
  if (rem_quirk) {
    if ((rem & 0x01) == 1) {
      rem = (rem + quot) / 2;
    }
  }
  result[4] = rem & 0xFF;
  result[5] = (rem >> 8) & 0xFF;
}

} // namespace VTxx
