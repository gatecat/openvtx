#include "irq.hpp"
#include "6502/mos6502.hpp"
#include "util.hpp"
#include <cassert>
#include <iostream>
namespace VTxx {
IRQController::IRQController(const vector<IRQVector> &_v,
                             mos6502::mos6502 *_cpu)
    : n(_v.size()), vectors(_v), cpu(_cpu) {
  status.resize(n, false);
};

void IRQController::write(uint8_t address, uint8_t data) {
  msk_reg = data;
  for (int i = 0; i < n; i++) // check this
    if (!get_bit(msk_reg, i))
      status[i] = false;
}
uint8_t IRQController::read(uint8_t address) { return msk_reg; }
void IRQController::set_irq(int idx, bool new_status) {
  assert(idx < n);
  if (!new_status) {
    status[idx] = false;
  } else {
    if (get_bit(msk_reg, idx)) {
      if (!status[idx]) {
        status[idx] = true;
        cout << "--- IRQ " << idx << " (0x" << hex << vectors[idx].h << ", 0x"
             << vectors[idx].l << ")" << endl;
        cpu->IRQ(vectors[idx].h, vectors[idx].l);
      }
    }
  }
}

} // namespace VTxx
