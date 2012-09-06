/*
 DMG startup ROM
 	LD SP,$fffe		    ; $0000  Setup Stack

	XOR A			    ; $0003  Zero the memory from $8000-$9FFF (VRAM)
	LD HL,$9fff		    ; $0004
Addr_0007:
	LD (HL-),A		    ; $0007
	BIT 7,H		        ; $0008
	JR NZ, Addr_0007	; $000a

	LD HL,$ff26		    ; $000c  Setup Audio
	LD C,$11		    ; $000f
	LD A,$80		    ; $0011
	LD (HL-),A		    ; $0013
	LD ($FF00+C),A	    ; $0014
	INC C			    ; $0015
	LD A,$f3		    ; $0016
	LD ($FF00+C),A	    ; $0018
	LD (HL-),A		    ; $0019
	LD A,$77		    ; $001a
	LD (HL),A		    ; $001c

	LD A,$fc		    ; $001d  Setup BG palette
	LD ($FF00+$47),A	; $001f

	LD DE,$0104		    ; $0021  Convert and load logo data from cart into Video RAM
	LD HL,$8010		    ; $0024
Addr_0027:
	LD A,(DE)		    ; $0027
	CALL $0095		    ; $0028
	CALL $0096		    ; $002b
	INC DE		        ; $002e
	LD A,E		        ; $002f
	CP $34		        ; $0030
	JR NZ, Addr_0027	; $0032

	LD DE,$00d8		    ; $0034  Load 8 additional bytes into Video RAM
	LD B,$08		    ; $0037
Addr_0039:
	LD A,(DE)		    ; $0039
	INC DE		        ; $003a
	LD (HL+),A		    ; $003b
	INC HL		        ; $003c
	DEC B			    ; $003d
	JR NZ, Addr_0039	; $003e

	LD A,$19		    ; $0040  Setup background tilemap
	LD ($9910),A	    ; $0042
	LD HL,$992f		    ; $0045
Addr_0048:
	LD C,$0c		    ; $0048
Addr_004A:
	DEC A			    ; $004a
	JR Z, Addr_0055	    ; $004b
	LD (HL-),A		    ; $004d
	DEC C			    ; $004e
	JR NZ, Addr_004A	; $004f
	LD L,$0f		    ; $0051
	JR Addr_0048	    ; $0053

	; === Scroll logo on screen, and play logo sound===

Addr_0055:
	LD H,A		        ; $0055  Initialize scroll count, H=0
	LD A,$64		    ; $0056
	LD D,A		; $0058  set loop count, D=$64
	LD ($FF00+$42),A	; $0059  Set vertical scroll register
	LD A,$91		    ; $005b
	LD ($FF00+$40),A   	; $005d  Turn on LCD, showing Background
	INC B			    ; $005f  Set B=1
Addr_0060:
	LD E,$02		    ; $0060
Addr_0062:
	LD C,$0c		    ; $0062
Addr_0064:
	LD A,($FF00+$44)	; $0064  wait for screen frame
	CP $90		        ; $0066
	JR NZ, Addr_0064	; $0068
	DEC C			    ; $006a
	JR NZ, Addr_0064	; $006b
	DEC E			    ; $006d
	JR NZ, Addr_0062	; $006e

	LD C,$13		    ; $0070
	INC H			    ; $0072  increment scroll count
	LD A,H		        ; $0073
	LD E,$83	        ; $0074
	CP $62		        ; $0076  $62 counts in, play sound #1
	JR Z, Addr_0080	    ; $0078
	LD E,$c1		    ; $007a
	CP $64		        ; $007c
	JR NZ, Addr_0086	; $007e  $64 counts in, play sound #2
Addr_0080:
	LD A,E		        ; $0080  play sound
	LD ($FF00+C),A	    ; $0081
	INC C			    ; $0082
	LD A,$87		    ; $0083
	LD ($FF00+C),A	    ; $0085
Addr_0086:
	LD A,($FF00+$42)	; $0086
	SUB B			    ; $0088
	LD ($FF00+$42),A	; $0089  scroll logo up if B=1
	DEC D			    ; $008b
	JR NZ, Addr_0060	; $008c

	DEC B			    ; $008e  set B=0 first time
	JR NZ, Addr_00E0	; $008f    ... next time, cause jump to "Nintendo Logo check"

	LD D,$20		    ; $0091  use scrolling loop to pause
	JR Addr_0060	    ; $0093

	; ==== Graphic routine ====

	LD C,A		        ; $0095  "Double up" all the bits of the graphics data
	LD B,$04		    ; $0096     and store in Video RAM
Addr_0098:
	PUSH BC		        ; $0098
	RL C			    ; $0099
	RLA			        ; $009b
	POP BC		        ; $009c
	RL C			    ; $009d
	RLA			        ; $009f
	DEC B			    ; $00a0
	JR NZ, Addr_0098	; $00a1
	LD (HL+),A		    ; $00a3
	INC HL		        ; $00a4
	LD (HL+),A		    ; $00a5
	INC HL		        ; $00a6
	RET			        ; $00a7

Addr_00A8:
	;Nintendo Logo
	.DB $CE,$ED,$66,$66,$CC,$0D,$00,$0B,$03,$73,$00,$83,$00,$0C,$00,$0D
	.DB $00,$08,$11,$1F,$88,$89,$00,$0E,$DC,$CC,$6E,$E6,$DD,$DD,$D9,$99
	.DB $BB,$BB,$67,$63,$6E,$0E,$EC,$CC,$DD,$DC,$99,$9F,$BB,$B9,$33,$3E

Addr_00D8:
	;More video data
	.DB $3C,$42,$B9,$A5,$B9,$A5,$42,$3C

	; ===== Nintendo logo comparison routine =====

Addr_00E0:
	LD HL,$0104		    ; $00e0	; point HL to Nintendo logo in cart
	LD DE,$00a8		    ; $00e3	; point DE to Nintendo logo in DMG rom

Addr_00E6:
	LD A,(DE)		    ; $00e6
	INC DE		        ; $00e7
	CP (HL)		        ; $00e8	;compare logo data in cart to DMG rom
	JR NZ,$fe		    ; $00e9	;if not a match, lock up here
	INC HL		        ; $00eb
	LD A,L		        ; $00ec
	CP $34		        ; $00ed	;do this for $30 bytes
	JR NZ, Addr_00E6	; $00ef

	LD B,$19		    ; $00f1
	LD A,B		        ; $00f3
Addr_00F4:
	ADD (HL)		    ; $00f4
	INC HL		        ; $00f5
	DEC B			    ; $00f6
	JR NZ, Addr_00F4	; $00f7
	ADD (HL)		    ; $00f9
	JR NZ,$fe		    ; $00fa	; if $19 + bytes from $0134-$014D  don't add to $00
						;  ... lock up

	LD A,$01		    ; $00fc
	LD ($FF00+$50),A	; $00fe	;turn off DMG rom
*/
void loadrom(unsigned char *mem)
{
    mem[0] = 0x31;
    mem[1] = 0xfe;
    mem[2] = 0xff;
    mem[3] = 0xaf;
    mem[4] = 0x21;
    mem[5] = 0xff;
    mem[6] = 0x9f;
    mem[7] = 0x32;
    mem[8] = 0xcb;
    mem[9] = 0x7c;
    mem[10] = 0x20;
    mem[11] = 0xfb;
    mem[12] = 0x21;
    mem[13] = 0x26;
    mem[14] = 0xff;
    mem[15] = 0x0e;
    mem[16] = 0x11;
    mem[17] = 0x3e;
    mem[18] = 0x80;
    mem[19] = 0x32;
    mem[20] = 0xe2;
    mem[21] = 0x0c;
    mem[22] = 0x3e;
    mem[23] = 0xf3;
    mem[24] = 0xe2;
    mem[25] = 0x32;
    mem[26] = 0x3e;
    mem[27] = 0x77;
    mem[28] = 0x77;
    mem[29] = 0x3e;
    mem[30] = 0xfc;
    mem[31] = 0xe0;
    mem[32] = 0x47;
    mem[33] = 0x11;
    mem[34] = 0x04;
    mem[35] = 0x01;
    mem[36] = 0x21;
    mem[37] = 0x10;
    mem[38] = 0x80;
    mem[39] = 0x1a;
    mem[40] = 0xcd;
    mem[41] = 0x95;
    mem[42] = 0x00;
    mem[43] = 0xcd;
    mem[44] = 0x96;
    mem[45] = 0x00;
    mem[46] = 0x13;
    mem[47] = 0x7b;
    mem[48] = 0xfe;
    mem[49] = 0x34;
    mem[50] = 0x20;
    mem[51] = 0xf3;
    mem[52] = 0x11;
    mem[53] = 0xd8;
    mem[54] = 0x00;
    mem[55] = 0x06;
    mem[56] = 0x08;
    mem[57] = 0x1a;
    mem[58] = 0x13;
    mem[59] = 0x22;
    mem[60] = 0x23;
    mem[61] = 0x05;
    mem[62] = 0x20;
    mem[63] = 0xf9;
    mem[64] = 0x3e;
    mem[65] = 0x19;
    mem[66] = 0xea;
    mem[67] = 0x10;
    mem[68] = 0x99;
    mem[69] = 0x21;
    mem[70] = 0x2f;
    mem[71] = 0x99;
    mem[72] = 0x0e;
    mem[73] = 0x0c;
    mem[74] = 0x3d;
    mem[75] = 0x28;
    mem[76] = 0x08;
    mem[77] = 0x32;
    mem[78] = 0x0d;
    mem[79] = 0x20;
    mem[80] = 0xf9;
    mem[81] = 0x2e;
    mem[82] = 0x0f;
    mem[83] = 0x18;
    mem[84] = 0xf3;
    mem[85] = 0x67;
    mem[86] = 0x3e;
    mem[87] = 0x64;
    mem[88] = 0x57;
    mem[89] = 0xe0;
    mem[90] = 0x42;
    mem[91] = 0x3e;
    mem[92] = 0x91;
    mem[93] = 0xe0;
    mem[94] = 0x40;
    mem[95] = 0x04;
    mem[96] = 0x1e;
    mem[97] = 0x02;
    mem[98] = 0x0e;
    mem[99] = 0x0c;
    mem[100] = 0xf0;
    mem[101] = 0x44;
    mem[102] = 0xfe;
    mem[103] = 0x90;
    mem[104] = 0x20;
    mem[105] = 0xfa;
    mem[106] = 0x0d;
    mem[107] = 0x20;
    mem[108] = 0xf7;
    mem[109] = 0x1d;
    mem[110] = 0x20;
    mem[111] = 0xf2;
    mem[112] = 0x0e;
    mem[113] = 0x13;
    mem[114] = 0x24;
    mem[115] = 0x7c;
    mem[116] = 0x1e;
    mem[117] = 0x83;
    mem[118] = 0xfe;
    mem[119] = 0x62;
    mem[120] = 0x28;
    mem[121] = 0x06;
    mem[122] = 0x1e;
    mem[123] = 0xc1;
    mem[124] = 0xfe;
    mem[125] = 0x64;
    mem[126] = 0x20;
    mem[127] = 0x06;
    mem[128] = 0x7b;
    mem[129] = 0xe2;
    mem[130] = 0x0c;
    mem[131] = 0x3e;
    mem[132] = 0x87;
    mem[133] = 0xe2;
    mem[134] = 0xf0;
    mem[135] = 0x42;
    mem[136] = 0x90;
    mem[137] = 0xe0;
    mem[138] = 0x42;
    mem[139] = 0x15;
    mem[140] = 0x20;
    mem[141] = 0xd2;
    mem[142] = 0x05;
    mem[143] = 0x20;
    mem[144] = 0x4f;
    mem[145] = 0x16;
    mem[146] = 0x20;
    mem[147] = 0x18;
    mem[148] = 0xcb;
    mem[149] = 0x4f;
    mem[150] = 0x06;
    mem[151] = 0x04;
    mem[152] = 0xc5;
    mem[153] = 0xcb;
    mem[154] = 0x11;
    mem[155] = 0x17;
    mem[156] = 0xc1;
    mem[157] = 0xcb;
    mem[158] = 0x11;
    mem[159] = 0x17;
    mem[160] = 0x05;
    mem[161] = 0x20;
    mem[162] = 0xf5;
    mem[163] = 0x22;
    mem[164] = 0x23;
    mem[165] = 0x22;
    mem[166] = 0x23;
    mem[167] = 0xc9;
    mem[168] = 0xce;
    mem[169] = 0xed;
    mem[170] = 0x66;
    mem[171] = 0x66;
    mem[172] = 0xcc;
    mem[173] = 0x0d;
    mem[174] = 0x00;
    mem[175] = 0x0b;
    mem[176] = 0x03;
    mem[177] = 0x73;
    mem[178] = 0x00;
    mem[179] = 0x83;
    mem[180] = 0x00;
    mem[181] = 0x0c;
    mem[182] = 0x00;
    mem[183] = 0x0d;
    mem[184] = 0x00;
    mem[185] = 0x08;
    mem[186] = 0x11;
    mem[187] = 0x1f;
    mem[188] = 0x88;
    mem[189] = 0x89;
    mem[190] = 0x00;
    mem[191] = 0x0e;
    mem[192] = 0xdc;
    mem[193] = 0xcc;
    mem[194] = 0x6e;
    mem[195] = 0xe6;
    mem[196] = 0xdd;
    mem[197] = 0xdd;
    mem[198] = 0xd9;
    mem[199] = 0x99;
    mem[200] = 0xbb;
    mem[201] = 0xbb;
    mem[202] = 0x67;
    mem[203] = 0x63;
    mem[204] = 0x6e;
    mem[205] = 0x0e;
    mem[206] = 0xec;
    mem[207] = 0xcc;
    mem[208] = 0xdd;
    mem[209] = 0xdc;
    mem[210] = 0x99;
    mem[211] = 0x9f;
    mem[212] = 0xbb;
    mem[213] = 0xb9;
    mem[214] = 0x33;
    mem[215] = 0x3e;
    mem[216] = 0x3c;
    mem[217] = 0x42;
    mem[218] = 0xb9;
    mem[219] = 0xa5;
    mem[220] = 0xb9;
    mem[221] = 0xa5;
    mem[222] = 0x42;
    mem[223] = 0x3c;
    mem[224] = 0x21;
    mem[225] = 0x04;
    mem[226] = 0x01;
    mem[227] = 0x11;
    mem[228] = 0xa8;
    mem[229] = 0x00;
    mem[230] = 0x1a;
    mem[231] = 0x13;
    mem[232] = 0xbe;
    mem[233] = 0x20;
    mem[234] = 0xfe;
    mem[235] = 0x23;
    mem[236] = 0x7d;
    mem[237] = 0xfe;
    mem[238] = 0x34;
    mem[239] = 0x20;
    mem[240] = 0xf5;
    mem[241] = 0x06;
    mem[242] = 0x19;
    mem[243] = 0x78;
    mem[244] = 0x86;
    mem[245] = 0x23;
    mem[246] = 0x05;
    mem[247] = 0x20;
    mem[248] = 0xfb;
    mem[249] = 0x86;
    mem[250] = 0x20;
    mem[251] = 0xfe;
    mem[252] = 0x3e;
    mem[253] = 0x01;
    mem[254] = 0xe0;
    mem[255] = 0x50;
}
