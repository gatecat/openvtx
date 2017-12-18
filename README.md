# OpenVTx Emulator for VR Tech Systems

OpenVTx is an emulator for the 6502-based, NES-inspired VR Technology based game systems. At the moment
it is a very early work in progress, currently having only limited PPU emulation, no sound output support
and most importantly no working input support. 

Currently the focus is on the VT168 processor and the MiWi2 Wii clone. Support for the VT368 and other
platforms will hopefully be added in the future.

At the moment it has only been tested on Linux, for which a Makefile is included. Once it is working well enough
to be released I will try and port it to Windows too. SDL is used for the graphics window and input.

OpenVTx contains a modified version of [mos6502](https://github.com/gianlucag/mos6502) by Gianluca Ghettini for the 6502 CPU core emulation.

To run OpenVTx, the following syntax should be used on the command line:

```
openvtx platform filename.bin
```

Where `platform` is the name of the platform (currently `vt168` for a minimal VT168 system or `miwi2` for the MiWi2), and
`filename.bin` is the path of the ROM to load.
