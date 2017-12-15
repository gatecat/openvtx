#ifndef EXTALU_H
#define EXTALU_H

#include <cstdint>
using namespace std;

namespace VTxx {
// VTxxx external MUL/DIV ALU
class ExtALU {
public:
  // Construct the ALU
  // rem_quirk enables VT168 broken remainder emulation
  // read_offset enables read addresses starting at 0x8 as in VT268+
  ExtALU(bool _rem_quirk, bool _read_offset);

  void write(uint8_t addr,
             uint8_t data); // address is 0..F  , relative to 0x2130
  uint8_t read(uint8_t addr);

private:
  void do_mul();
  void do_div();
  uint8_t operand[4];
  uint8_t mul_operand[2];
  uint8_t div_operand[2];
  uint8_t result[6];

  bool rem_quirk;
  bool read_offset;
};
} // namespace VTxx

#endif /* end of include guard: EXTALU_H */
