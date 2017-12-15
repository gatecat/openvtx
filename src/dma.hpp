#ifndef DMA_HPP
#define DMA_HPP
#include <cstdint>
using namespace std;
namespace VTxx {
class DMACtrl {
public:
  DMACtrl();

  void write(uint8_t addr,
             uint8_t data); // address is 0..6, relative to 0x2122
  uint8_t read(uint8_t addr);

  void vblank_notify(); // notify the DMA engine of the start of VBLANK

private:
  bool waiting_vblank = false;
  uint8_t dma_regs[7] = {0};
  void do_xfer();
  bool is_vram_xfer();
  inline bool is_busy() { return waiting_vblank; }
  inline uint16_t get_src_addr() { return (dma_regs[2] << 8UL) | dma_regs[3]; }
  inline uint16_t get_dst_addr() { return (dma_regs[1] << 8UL) | dma_regs[0]; }
};

} // namespace VTxx

#endif /* end of include guard: DMA_HPP */
