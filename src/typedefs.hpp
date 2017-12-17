#ifndef TYPEDEFS_H
#define TYPEDEFS_H
#include <cstdint>
using namespace std;
namespace VTxx {
// Timer IRQ callback - 1 signals IRQ fire and 0 signals IRQ clear
typedef void (*TimerCallback)(bool status);

// Some control registers have special handlers for read or write
// These are the types for these
typedef uint8_t (*ReadHandler)(uint16_t addr);
typedef void (*WriteHandler)(uint16_t addr, uint8_t value);

} // namespace VTxx

#endif /* end of include guard: TYPEDEFS_H */
