# OpenVTx Emulator for VR Tech Systems

OpenVTx is an emulator for the 6502-based, NES-inspired VR Technology based game systems. At the moment
it is an early work in progress, currently having some PPU rendering issues and no sound emulation.

Currently the focus is on the VT168 processor and the MiWi2 Wii clone. Support for the VT368 and other
platforms will hopefully be added in the future.

At the moment it has only been tested on Linux, for which a Makefile is included. Once it is working well enough
to be released I will try and port it to Windows too. SDL is used for the graphics window and input.

OpenVTx contains a modified version of [mos6502](https://github.com/gianlucag/mos6502) by Gianluca Ghettini for the 6502 CPU core emulation.

To run OpenVTx on the command line, the following syntax can be used:
openvtx platform filename.bin

Where `platform` is the name of the platform (currently `vt168` for a minimal VT168 system or `miwi2` for the MiWi2), and
`filename.bin` is the path of the ROM to load.

A simple WxWidgets GUI will be used for platform and ROM selection if it is run without arguments.

The key bindings are as follows:
 - Up/Down/Left/Right cursor keys map to the D-pad
 - Enter maps to start and R-Shift maps to select
 - Z maps to B and X maps to A
 - R is a soft reset (possibly buggy)
 
# Known Issues
 - No sound emulation (SCPU is partially emulated but no sound output support)
 - The road is missing (corruped background layer is visible instead) in the "3D" perspective racing games
 - Poor input device emulation means only the first two games can be selected in the InterAct 8-in-1 ROM
 
# Supported ROMs
  - VRT VT1682 demo ROM (use `vt168` platform)
  - MiWi 2 Sports 7 in 1 (use `miwi2` platform)
  - MiWi 2 16 Arcade Games + Drum Master (use `miwi2` platform)
  - InterAct 8-in-1 (use `miwi2` platform, note above input issue)
  - InterAct 32-in-1 (use `miwi2` platform)

# Legal

Copyright (c) 2017 David Shah

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

If you are using the Windows binary of OpenVTx, please note that it is
statically linked with unmodified versions of SDL2 and WxWidgets.
