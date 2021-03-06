#include <stdio.h>
#include <stdint.h>
#include "memorymap.h"
#include "rom.inl"          //loading the boot rom into memory - keep this in a separate file for now.

int error = 0;

static uint16_t rAF;
static uint16_t rBC;
static uint16_t rDE;
static uint16_t rHL;
static uint16_t rSP;
static uint16_t rPC;

unsigned char *const rA = ((unsigned char*)&rAF) + 1;
unsigned char *const rF = (unsigned char*)&rAF;     //watch endianness!
unsigned char *const rB = ((unsigned char*)&rBC) + 1;
unsigned char *const rC = (unsigned char*)&rBC;
unsigned char *const rD = ((unsigned char*)&rDE) + 1;
unsigned char *const rE = (unsigned char*)&rDE;
unsigned char *const rH = ((unsigned char*)&rHL) + 1;
unsigned char *const rL = (unsigned char*)&rHL;

#define FLAG_Z 7
#define FLAG_N 6
#define FLAG_H 5
#define FLAG_C 4
#define BIT_Z 0x80
#define BIT_N 0x40
#define BIT_H 0x20
#define BIT_C 0x10
#define COND_NZ 0x0
#define COND_Z 0x1
#define COND_NC 0x2
#define COND_C 0x3

#define SETZEROFLAG(reg) setbit(rF, FLAG_Z, *(reg) == 0)            //sets the zero flag based on the contents of the given register (more DRY)

unsigned char* regLookup[8] = //{rB, rC, rD, rE, rH, rL, rF, rA};     //rF should never actually be accessed; 110 should not be a valid register operand!
{
    ((unsigned char*)&rBC) + 1,
    (unsigned char*)&rBC,
    ((unsigned char*)&rDE) + 1,
    (unsigned char*)&rDE,
    ((unsigned char*)&rHL) + 1,
    (unsigned char*)&rHL,
    (unsigned char*)&rAF,
    ((unsigned char*)&rAF) + 1      //watch endianness! (for little endian the low byte is at the front)
};

uint16_t* dregLookup[4] =
{
    &rBC,
    &rDE,
    &rHL,
    &rSP
};

inline void setbit(unsigned char *byte, int field, int true)
{
	*byte &= ~(1 << field);							//field goes low to high, i.e. right to left
	*byte |= true << field;
}

inline void setbits(unsigned char *byte, unsigned char notmask, unsigned char bits)			//notmask: 1 if the bit should be cleared, otherwise 0.
{
	*byte = (*byte & ~notmask) | bits;
}

inline int getbit(unsigned char byte, int field)
{
	return (byte >> field) & 0x01;
}

inline int testCondition(int condtype)      // inputs cc bits: should be a number between 0 and 7
{
    switch (condtype)
    {
        case COND_NZ:
            return !getbit(*rF, FLAG_Z);
        case COND_Z:
            return getbit(*rF, FLAG_Z);
        case COND_NC:
            return !getbit(*rF, FLAG_C);
        case COND_C:
            return getbit(*rF, FLAG_C);
        default:
            printf("Error: invalid condition field 0x%x\n", condtype);
            error = 1;
            return 0;
    }
}

void initgb()
{
    printf("Loading rom image... ");
    loadrom(mem);
    printf("Done!\n");
    rPC = 0;
}

inline void step()
{
    unsigned char opcode = mem[rPC];

/// 8 bit loads:
    if ((opcode & 0xc0) == 0x40)                //first two bits are 01: this is an ld r, r instruction
    {
        if ((opcode & 0xf8) == 0x70)            //ld (HL), r
            mem[rHL] = *regLookup[opcode & 0x07];
        else if ((opcode & 0x07) == 0x06)       //ld r, (HL)
            *regLookup[(opcode >> 3) & 0x7] = mem[rHL];
        else
            *regLookup[(opcode >> 3) & 0x7] = *regLookup[opcode & 0x07];        //bitfields: 0|1|r|r|r|R|R|R
    }
    else if ((opcode & 0xc7) == 0x06)          // ld r, n: 0|0|r|r|r|1|1|0, nnnnnnnn
    {
        *regLookup[(opcode >> 3) & 0x07] = mem[++rPC];
    }
/// 16 bit loads:
    else if ((opcode & 0xcf) == 0x01)           // ld dd, nn: 0|0|d|d|0|0|0|1, n^16
    {
        uint16_t result = mem[++rPC];
        result = result | (mem[++rPC] << 8);    /// take note: Little Endian!
        *dregLookup[(opcode >> 4) & 0x3] = result;
    }
    else if ((opcode & 0xcb) == 0xc1)           // 1|1|d|d|0|p|0|1; p indicates push/pop.
    {
        if (opcode & 0x04)                      // push dd: 1|1|d|d|0|1|0|1
        {
            rSP -= 2;
            uint16_t contents = *dregLookup[(opcode >> 4) & 0x3];
            mem[rSP] = contents & 0xff;
            mem[rSP + 1] = contents >> 8;       // little endian! low byte is stored first.
        }
        else                                    // pop dd:  1|1|d|d|0|0|0|1
        {
            uint16_t *reg = dregLookup[(opcode >> 4) & 0x03];
            *reg = mem[rSP++];
            *reg |= mem[rSP++] << 8;
        }
    }
/// 8 bit arithmetic:
    else if ((opcode & 0xf8) == 0x80)
    {
        if ((opcode & 0x07) == 0x06)            // add A, (HL)
            *rA += mem[rHL];
        else
            *rA += *regLookup[opcode & 0x07];   // add A, r: 1|0|0|0|0|r|r|r
		SETZEROFLAG(rA);
    }
    else if ((opcode & 0xf8) == 0x90)
    {
        if ((opcode & 0x07) == 0x06)            // sub A, (HL)
            *rA -= mem[rHL];
        else
            *rA -= *regLookup[opcode & 0x07];   // sub A, r: 1|0|0|0|1|r|r|r
		SETZEROFLAG(rA);
	}
    else if ((opcode & 0xf8) == 0xa0)
    {
        if ((opcode & 0x07) == 0x06)            // and A, (HL)
            *rA &= mem[rHL];
        else
            *rA &= *regLookup[opcode & 0x07];   // and A, r: 1|0|1|0|0|r|r|r
		SETZEROFLAG(rA);
 		setbits(rF, BIT_N | BIT_H | BIT_C, 0);
   }
    else if ((opcode & 0xf8) == 0xb0)
    {
        if ((opcode & 0x07) == 0x06)            // or A, (HL)
            *rA |= mem[rHL];
        else
            *rA |= *regLookup[opcode & 0x07];   // or A, r: 1|0|1|0|0|r|r|r
		SETZEROFLAG(rA);
		setbits(rF, BIT_N | BIT_H | BIT_C, 0);
    }
    else if ((opcode & 0xf8) == 0xa8)
    {
        if ((opcode & 0x07) == 0x06)            // xor A, (HL)
            *rA ^= mem[rHL];
        else
            *rA ^= *regLookup[opcode & 0x07];   // xor A, r: 1|0|1|0|0|r|r|r
		SETZEROFLAG(rA);
		setbits(rF, BIT_N | BIT_H | BIT_C, 0);
    }
    else if ((opcode & 0xc7) == 0x04)           // inc r: 0|0|r|r|r|1|0|0
    {
        unsigned char *reg = (opcode & 0x38) == 0x30 ? &mem[rHL] : regLookup[(opcode >> 3) & 0x7];          // rrr == 110; inc (HL)
        (*reg)++;
        SETZEROFLAG(reg);
    }
    else if ((opcode & 0xc7) == 0x05)           // dec r: 0|0|r|r|r|1|0|1
    {
        unsigned char *reg = (opcode & 0x38) == 0x30 ? &mem[rHL] : regLookup[(opcode >> 3) & 0x7];          // rrr == 110; dec (HL)
        (*reg)--;
        SETZEROFLAG(reg);
    }
    else if ((opcode & 0xf8) == 0xb8 || opcode == 0xfe)
    {
        unsigned char val;
        if (opcode == 0xfe)                     // cp n
        {
            val = mem[++rPC];
        }
        else                                    // cp r: 1|0|1|1|1|r|r|r        (r == 110 => (HL))
        {
            val = (opcode & 0x07) == 0x06 ? mem[rHL] : *regLookup[opcode & 0x07];
        }
        setbit(rF, FLAG_Z, *rA == val);
        setbit(rF, FLAG_C, *rA < val);
        setbit(rF, FLAG_N, 1);
    }
/// 16-bit arithmetic
    else if ((opcode & 0xc7) == 0x03)           // 0|0|s|s|d|0|1|1 (d indicates decrement vs. increment)
    {
        uint16_t *reg = dregLookup[(opcode >> 4) & 0x03];
        if (opcode & 0x08)                      // dec ss: 0|0|s|s|1|0|1|1
        {
            (*reg)--;
        }
        else                                    // inc ss: 0|0|s|s|0|0|1|1
        {
            (*reg)++;
        }
        SETZEROFLAG(reg);
    }
/// absolute jumps
    else if ((opcode & 0xe7) == 0xc2 || opcode == 0xc3)             // JP [cc, ] nn: 1|1|0|c|c|0|1|C  n^16    (C indicates that the jump is non-conditional)
    {                                                               // in Z80 this is1|1|c|c|c|0|1|C ; however GB only has four conditions, and opcodes with a one for the first c are now used otherwise.
        if (opcode & 0x1 || testCondition((opcode >> 3) & 0x07))    // either non-conditional or the condition returns true
        {
            uint16_t address = mem[++rPC];
            address |= mem[++rPC] << 8;
            rPC = address;
            rPC--;                              // <- because it gets decremented later; otherwise we'd need a branch, not much better :')
        }
        else
        {
            rPC += 2;                           // skip the address bytes.
        }
    }
/// relative jumps
	else if ((opcode & 0xe7) == 0x20)			// jr cond, dd:  0|0|1|c|c|0|0|0 d^8   where cc is cond bits
	{
		if (testCondition((opcode >> 3) & 0x3))
		{
			rPC += *((signed char*) &mem[++rPC]);
		}
		else
		{
			++rPC;
		}
	}
/// function calls/returns
	else if ((opcode & 0xc7) == 0xc4)			// call cc, nn: 1|1|c|c|c|1|1|0
	{											// equivalent: SP = SP - 2,  (SP) = PC, PC = nn
		if (testCondition((opcode >> 3) & 0x7))
		{
			rSP -= 2;
			uint16_t address = mem[++rPC];
			mem[rSP] = rPC;
			mem[rSP + 1] = rPC >> 8;            // we store rPC *after* reading the address - otherwise we jump back into the address bytes!
			address |= mem[++rPC] << 8;
			printf("Calling procedure at address 0x%04x\n", address);
			rPC = address - 1;                  // <- -1 because it gets incremented at the end of the cycle
		}
		else
		{
			rPC += 2;
		}
	}
	else if ((opcode & 0xe7) == 0xc0)			// ret cc: 1|1|0|c|c|0|0|0   (note that there are only two condition bits: in Z80 there would be another preceding bit)
	{											// equivalent: PC = (SP), SP = SP + 2
		if (testCondition((opcode >> 3) & 0x7))
		{
			rPC = mem[rSP];
			rPC |= mem[rSP + 1] << 8;           // high byte is second in memory
			rSP += 2;
		}
	}
    else
    {
        switch(opcode)
        {
            case 0x07:                          // rlca - rotate accu left
                *rA = ((*rA) << 1) | ((*rA) >> 7);
                setbit(rF, FLAG_C, (*rA) & 0x01);
                break;
            case 0x17:                          // rla - rotate accu left through carry
            {
                char carrybit = getbit(*rF, FLAG_C);
                setbit(rF, FLAG_C, (*rA) >> 7);
                *rA = ((*rA) << 1) | carrybit;
                break;
            }
            case 0x0f:                          // rrca - rotate accu right
                setbit(rF, FLAG_C, (*rA) & 0x01);
                *rA = ((*rA) >> 1) | ((*rA) << 7);
                break;
            case 0x1f:                          // rra - rotate accu right through carry
            {
                char carrybit = getbit(*rF, FLAG_C);
                setbit(rF, FLAG_C, (*rA) & 0x01);
                *rA = ((*rA) >> 1) | (carrybit << 7);
                break;
            }
            case 0x36:                          // ld A, nn
                mem[rHL] = mem[++rPC];
                break;
            case 0x0a:
                *rA = mem[rBC];                 // ld A, (BC)
                break;
            case 0x1a:
                *rA = mem[rDE];                 // ld A, (DE)
                break;
            case 0xfa:                          // ld A, (nn)
            {
                unsigned char partaddress = mem[++rPC];
                *rA = mem[partaddress | (mem[++rPC] << 8)];
                break;
            }
            case 0x02:                          // ld (BC), A
                mem[rBC] = *rA;
                break;
            case 0x12:                          // ld (DE), A
                mem[rDE] = *rA;
                break;
            case 0xea:                          // ld (nn), A
            {
                unsigned char partaddress = mem[++rPC];
                mem[partaddress | mem[++rPC] << 8] = *rA;
                break;
            }
            case 0xe0:                          // ld (FF00 + nn), A     (write to io port nn)
                mem[0xff00 + mem[++rPC]] = *rA;
                break;
            case 0xe2:                          // ld (FF00 + C), A     (write to io port C)
                mem[0xff00 + *rC] = *rA;
                break;
            case 0xf0:                          // ld A, (FF00 + nn)     (read from io port nn)
                *rA = mem[0xff00 + mem[++rPC]];
                break;
            case 0xf2:                          // ld A, (FF00 + C)     (read from io port C)
                *rA = mem[0xff00 + *rC];
                break;
            case 0x22:                          // ldi (HL), A
                mem[rHL++] = *rA;
                break;
            case 0x2a:                          // ldi A, (HL)
                *rA = mem[rHL++];
                break;
            case 0x32:                          // ldd (HL), A
                mem[rHL--] = *rA;
                break;
            case 0x3a:                          // ldd A, (HL)
                *rA = mem[rHL--];
                break;
			case 0xCB:
/// single bit commands
				opcode = mem[++rPC];			// we're not interested in the prefix any more! (load next byte)
                if ((opcode & 0xc0) == 0x40)
                {
                    if ((opcode & 0x07) == 0x06)// bit b, (HL): 0|1|b|b|b|1|1|0
                        setbit(rF, FLAG_Z, getbit(mem[rHL], (opcode >> 3) & 0x07) == 0);
                    else                        // bit b, r: 0|1|b|b|b|r|r|r
                        setbit(rF, FLAG_Z, getbit(*regLookup[opcode & 0x07], (opcode >> 3) & 0x07) == 0);   // fetch the requested bit and set the zero flag based on the result
                    setbits(rF, BIT_N | BIT_H, BIT_H);      //affected flags: z01- (out of z, n, h, c)
                }
                else if ((opcode & 0xc0) == 0xc0)
                {
                    if ((opcode & 0x07) == 0x06)// set b, (HL): 1|1|b|b|b|1|1|0
                        setbit(&mem[rHL], (opcode >> 3) & 0x07, 1);
                    else                        // set b, r: 1|1|b|b|b|r|r|r
                        setbit(regLookup[opcode & 0x07], (opcode >> 3) & 0x07, 1);
                }
                else if ((opcode & 0xc0) == 0x80)
                {
                    if ((opcode & 0x07) == 0x06)// res b, (HL): 1|0|b|b|b|1|1|0
                        setbit(&mem[rHL], (opcode >> 3) & 0x07, 0);
                    else                        // res b, r: 1|0|b|b|b|r|r|r
                        setbit(regLookup[opcode & 0x07], (opcode >> 3) & 0x07, 0);
                }
/// rotate-and-shift commands
                else if((opcode & 0xf8) == 0x00)// rlc - rotate left: 0|0|0|0|0|r|r|r
                {
                    unsigned char *reg;
                    if ((opcode & 0x07) == 0x06)
                        reg = &mem[rHL];
                    else
                        reg = regLookup[opcode & 0x07];
                    *reg = ((*reg) << 1) | ((*reg) >> 7);
                    setbit(rF, FLAG_C, (*reg) & 0x01);
                }
                else if((opcode & 0xf8) == 0x10)// rl - rotate left through carry
                {
                    unsigned char *reg;
                    if ((opcode & 0x07) == 0x06)
                        reg = &mem[rHL];
                    else
                        reg = regLookup[opcode & 0x07];
                    char carrybit = getbit(*rF, FLAG_C);
                    setbit(rF, FLAG_C, (*reg) >> 7);
                    *reg = ((*reg) << 1) | carrybit;
                }
                else if((opcode & 0xf8) == 0x08)// rrc - rotate right
                {
                    unsigned char *reg;
                    if ((opcode & 0x07) == 0x06)
                        reg = &mem[rHL];
                    else
                        reg = regLookup[opcode & 0x07];
                    setbit(rF, FLAG_C, (*reg) & 0x08);
                    *reg = ((*reg) >> 1) | ((*reg) << 7);
                }
                else if((opcode & 0xf8) == 0x0)// rr - rotate right through carry
                {
                    unsigned char *reg;
                    if ((opcode & 0x07) == 0x06)
                        reg = &mem[rHL];
                    else
                        reg = regLookup[opcode & 0x07];
                    char carrybit = getbit(*rF, FLAG_C);
                    setbit(rF, FLAG_C, (*reg) & 0x18);
                    *reg = ((*reg) >> 1) | (carrybit << 7);
                }
                else
                {
                    printf("Unrecognized CB-prefixed instruction: 0x%02x (Memory location: 0x%04x)\n", opcode, rPC);
                    error = 1;
                }
				break;
/// jumps
            case 0xc3:      // jp nn: 0xc3, n^16
            {
                uint16_t address = mem[++rPC];
                address |= mem[++rPC] << 8;
                rPC = address - 1;
                break;
            }
            case 0xe9:      // jp HL
                rPC = rHL - 1;
				break;
			case 0x18:      // jr nn
				rPC += *((signed char*) &mem[++rPC]);
				break;
			case 0xcd:		// call nn   (n^16)
				rSP -= 2;
				uint16_t address = mem[++rPC];
				address |= mem[++rPC] << 8;
				mem[rSP] = rPC;
				mem[rSP + 1] = rPC >> 8;
                rPC = address - 1;                  // <- -1 because it gets incremented at the end of the cycle
				break;
			case 0xc9:		// ret: 1|1|0|0|1|0|0|1
				rPC = mem[rSP];
				rPC |= mem[rSP + 1] << 8;           // high byte comes second.
				rSP += 2;
				break;
/// CPU control
			case 0x3f:		// ccf (change carry flag)
				*rF ^= BIT_C;
				break;
			case 0x37:		// scf (set carry flag)
				*rF |= BIT_C;
				break;
			case 0x00:		// nop
				break;
			case 0x76:		// halt
				printf("HALT\n");
				error = 1;
				break;

            case 0x10:
                printf("STOP\n");
                error = 1;
                break;
            default:
                printf("Unrecognized instruction: 0x%02x (Memory location: 0x%04x)\n", opcode, rPC);
                error = 1;
                break;
        }

    }
    rPC++;
}

int main()
{
    initgb();
    while (!error)
    {
        printf("Instruction: 0x%02x (Memory location: 0x%04x)\n", mem[rPC], rPC);
        //getchar();
        step();
    }
    printf("AF: 0x%04x\nBC: 0x%04x\nDE: 0x%04x\nHL: 0x%04x\nSP: 0x%04x\n", rAF, rBC, rDE, rHL, rSP);
    return 0;
}
