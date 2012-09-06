#ifndef MEMORYMAP_H_INCLUDED
#define MEMORYMAP_H_INCLUDED
/* Pan Docs memory map:
 * 0000-3FFF   16KB ROM Bank 00     (in cartridge, fixed at bank 00)
 * 4000-7FFF   16KB ROM Bank 01..NN (in cartridge, switchable bank number)
 * 8000-9FFF   8KB Video RAM (VRAM) (switchable bank 0-1 in CGB Mode)
 * A000-BFFF   8KB External RAM     (in cartridge, switchable bank, if any)
 * C000-CFFF   4KB Work RAM Bank 0 (WRAM)
 * D000-DFFF   4KB Work RAM Bank 1 (WRAM)  (switchable bank 1-7 in CGB Mode)
 * E000-FDFF   Same as C000-DDFF (ECHO)    (typically not used)
 * FE00-FE9F   Sprite Attribute Table (OAM)
 * FEA0-FEFF   Not Usable
 * FF00-FF7F   I/O Ports
 * FF80-FFFE   High RAM (HRAM)
 * FFFF        Interrupt Enable Register
 */
static unsigned char mem[0x10000];

unsigned char *rombank00 = mem + 0;
unsigned char *rombank01 = mem + 0x4000;
unsigned char *vram      = mem + 0x8000;
unsigned char *exram     = mem + 0xA000;
unsigned char *wram0     = mem + 0xC000;
unsigned char *wram1     = mem + 0xD000;
//ECHO
unsigned char *oam       = mem + 0xfe00;
unsigned char *ioports   = mem + 0xff00;
unsigned char *hram      = mem + 0xff80;
unsigned char *intr      = mem + 0xffff;

#endif // MEMORYMAP_H_INCLUDED
