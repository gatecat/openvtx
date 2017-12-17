#ifndef IRQ_H
#define IRQ_H
#include <cstdint>
#include <vector>
using namespace std;

namespace mos6502 {
class mos6502;
};

namespace VTxx {

struct IRQVector {
  uint16_t h, l;
};

class IRQController {
public:
  IRQController(const vector<IRQVector> &_v, mos6502::mos6502 *_cpu);
  // Only 1 address, 0 is the mask register
  void write(uint8_t address, uint8_t data);
  uint8_t read(uint8_t address);
  void set_irq(int idx, bool new_status);

private:
  uint8_t msk_reg = 0;
  int n;
  vector<IRQVector> vectors;
  vector<bool> status;
  mos6502::mos6502 *cpu;
};

}; // namespace VTxx

#endif /* end of include guard: IRQ_H */
