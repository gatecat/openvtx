#ifndef SCPU_H
#define SCPU_H
#include "typedefs.hpp"
#include <cstdint>
using namespace std;

namespace VTxx {
// The system control registers, 0x2100 .. 0x21FF
extern uint8_t scpu_control_reg[256];

uint8_t scpu_read_mem(uint16_t addr);
void scpu_write_mem(uint16_t addr, uint8_t data);

// Read and write handlers for SCPU register space
extern ReadHandler scpu_reg_read_fn[256];
extern WriteHandler scpu_reg_write_fn[256];

} // namespace VTxx

#endif /* end of include guard: SCPU_H */
