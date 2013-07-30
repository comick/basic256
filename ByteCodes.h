/** Copyright (C) 2006, Ian Paul Larsen.
 **
 **  This program is free software; you can redistribute it and/or modify
 **  it under the terms of the GNU General Public License as published by
 **  the Free Software Foundation; either version 2 of the License, or
 **  (at your option) any later version.
 **
 **  This program is distributed in the hope that it will be useful,
 **  but WITHOUT ANY WARRANTY; without even the implied warranty of
 **  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 **  GNU General Public License for more details.
 **
 **  You should have received a copy of the GNU General Public License along
 **  with this program; if not, write to the Free Software Foundation, Inc.,
 **  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 **/




//No argument ops GE OP_TYPEARGNONE and LT OP_TYPEARGINT
// RANGE 0x00 to 0x7F

#define OP_TYPEARGNONE		0x00
#define OP_END   			OP_TYPEARGNONE + 0x00
#define OP_NOP   			OP_TYPEARGNONE + 0x01
#define OP_RETURN			OP_TYPEARGNONE + 0x02
#define OP_CONCAT			OP_TYPEARGNONE + 0x03
#define OP_EQUAL 			OP_TYPEARGNONE + 0x04
#define OP_NEQUAL			OP_TYPEARGNONE + 0x05
#define OP_GT    			OP_TYPEARGNONE + 0x06
#define OP_LT    			OP_TYPEARGNONE + 0x07
#define OP_GTE   			OP_TYPEARGNONE + 0x08
#define OP_LTE   			OP_TYPEARGNONE + 0x09
#define OP_AND   			OP_TYPEARGNONE + 0x0a
#define OP_NOT   			OP_TYPEARGNONE + 0x0b
#define OP_OR    			OP_TYPEARGNONE + 0x0c
#define OP_XOR   			OP_TYPEARGNONE + 0x0d
#define OP_INT   			OP_TYPEARGNONE + 0x0e
#define OP_STRING			OP_TYPEARGNONE + 0x0f
#define OP_ADD   			OP_TYPEARGNONE + 0x10
#define OP_SUB   			OP_TYPEARGNONE + 0x11
#define OP_MUL   			OP_TYPEARGNONE + 0x12
#define OP_DIV   			OP_TYPEARGNONE + 0x13
#define OP_EX	 			OP_TYPEARGNONE + 0x14
#define OP_NEGATE			OP_TYPEARGNONE + 0x15
#define OP_PRINT 			OP_TYPEARGNONE + 0x16
#define OP_PRINTN			OP_TYPEARGNONE + 0x17
#define OP_INPUT 			OP_TYPEARGNONE + 0x18
#define OP_KEY   			OP_TYPEARGNONE + 0x19
#define OP_PLOT  			OP_TYPEARGNONE + 0x1a
#define OP_RECT  			OP_TYPEARGNONE + 0x1b
#define OP_CIRCLE			OP_TYPEARGNONE + 0x1c
#define OP_LINE  			OP_TYPEARGNONE + 0x1d
#define OP_REFRESH			OP_TYPEARGNONE + 0x1e
#define OP_FASTGRAPHICS		OP_TYPEARGNONE + 0x1f
#define OP_CLS				OP_TYPEARGNONE + 0x20
#define OP_CLG				OP_TYPEARGNONE + 0x21
#define OP_GRAPHSIZE		OP_TYPEARGNONE + 0x22
#define OP_GRAPHWIDTH		OP_TYPEARGNONE + 0x23
#define OP_GRAPHHEIGHT		OP_TYPEARGNONE + 0x24
#define OP_SIN				OP_TYPEARGNONE + 0x25
#define OP_COS				OP_TYPEARGNONE + 0x26
#define OP_TAN				OP_TYPEARGNONE + 0x27
#define OP_RAND				OP_TYPEARGNONE + 0x28
#define OP_CEIL				OP_TYPEARGNONE + 0x29
#define OP_FLOOR			OP_TYPEARGNONE + 0x2a
#define OP_ABS				OP_TYPEARGNONE + 0x2b
#define OP_PAUSE			OP_TYPEARGNONE + 0x2c
#define OP_POLY				OP_TYPEARGNONE + 0x2d
#define OP_LENGTH			OP_TYPEARGNONE + 0x2e
#define OP_MID				OP_TYPEARGNONE + 0x2f
#define OP_INSTR			OP_TYPEARGNONE + 0x30
#define OP_INSTR_S			OP_TYPEARGNONE + 0x31
#define OP_INSTR_SC			OP_TYPEARGNONE + 0x32
#define OP_INSTRX			OP_TYPEARGNONE + 0x33
#define OP_INSTRX_S			OP_TYPEARGNONE + 0x34
#define OP_OPEN				OP_TYPEARGNONE + 0x35
#define OP_READ				OP_TYPEARGNONE + 0x36
#define OP_WRITE			OP_TYPEARGNONE + 0x37
#define OP_CLOSE			OP_TYPEARGNONE + 0x38
#define OP_RESET			OP_TYPEARGNONE + 0x39
#define OP_SOUND			OP_TYPEARGNONE + 0x3a
#define OP_INCREASERECURSE	OP_TYPEARGNONE + 0x3b
#define OP_DECREASERECURSE	OP_TYPEARGNONE + 0x3c
#define OP_ASC				OP_TYPEARGNONE + 0x3d
#define OP_CHR				OP_TYPEARGNONE + 0x3e
#define OP_FLOAT			OP_TYPEARGNONE + 0x3f
#define OP_READLINE			OP_TYPEARGNONE + 0x40
#define OP_EOF				OP_TYPEARGNONE + 0x41
#define OP_MOD				OP_TYPEARGNONE + 0x42
#define OP_YEAR				OP_TYPEARGNONE + 0x43
#define OP_MONTH			OP_TYPEARGNONE + 0x44
#define OP_DAY				OP_TYPEARGNONE + 0x45
#define OP_HOUR				OP_TYPEARGNONE + 0x46
#define OP_MINUTE			OP_TYPEARGNONE + 0x47
#define OP_SECOND			OP_TYPEARGNONE + 0x48
#define OP_MOUSEX			OP_TYPEARGNONE + 0x49
#define OP_MOUSEY			OP_TYPEARGNONE + 0x4a
#define OP_MOUSEB			OP_TYPEARGNONE + 0x4b
#define OP_CLICKCLEAR		OP_TYPEARGNONE + 0x4c
#define OP_CLICKX			OP_TYPEARGNONE + 0x4d
#define OP_CLICKY			OP_TYPEARGNONE + 0x4e
#define OP_CLICKB			OP_TYPEARGNONE + 0x4f
//OP_TYPEARGNONE + 0x50 free
#define OP_TEXT				OP_TYPEARGNONE + 0x51
#define OP_FONT				OP_TYPEARGNONE + 0x52
#define OP_SAY				OP_TYPEARGNONE + 0x53
#define OP_WAVPLAY			OP_TYPEARGNONE + 0x54
#define OP_WAVSTOP			OP_TYPEARGNONE + 0x55
#define OP_SEEK				OP_TYPEARGNONE + 0x56
#define OP_SIZE				OP_TYPEARGNONE + 0x57
#define OP_EXISTS			OP_TYPEARGNONE + 0x58
#define OP_LEFT				OP_TYPEARGNONE + 0x59
#define OP_RIGHT			OP_TYPEARGNONE + 0x5a
#define OP_UPPER			OP_TYPEARGNONE + 0x5b
#define OP_LOWER			OP_TYPEARGNONE + 0x5c
#define OP_SYSTEM			OP_TYPEARGNONE + 0x5d
#define OP_VOLUME			OP_TYPEARGNONE + 0x5e
#define OP_SETCOLOR			OP_TYPEARGNONE + 0x5f
#define OP_RGB				OP_TYPEARGNONE + 0x60
#define OP_PIXEL			OP_TYPEARGNONE + 0x61
#define OP_GETCOLOR			OP_TYPEARGNONE + 0x62
#define OP_ASIN				OP_TYPEARGNONE + 0x63
#define OP_ACOS				OP_TYPEARGNONE + 0x64
#define OP_ATAN				OP_TYPEARGNONE + 0x65
#define OP_DEGREES			OP_TYPEARGNONE + 0x66
#define OP_RADIANS			OP_TYPEARGNONE + 0x67
#define OP_INTDIV 	       	OP_TYPEARGNONE + 0x68
#define OP_LOG				OP_TYPEARGNONE + 0x69
#define OP_LOGTEN 	       	OP_TYPEARGNONE + 0x6a
#define OP_GETSLICE			OP_TYPEARGNONE + 0x6b
#define OP_PUTSLICE			OP_TYPEARGNONE + 0x6c
#define OP_PUTSLICEMASK		OP_TYPEARGNONE + 0x6d
#define OP_IMGLOAD 	       	OP_TYPEARGNONE + 0x6e
#define OP_SQR				OP_TYPEARGNONE + 0x6f
#define OP_EXP				OP_TYPEARGNONE + 0x70
#define OP_ARGUMENTCOUNTTEST	OP_TYPEARGNONE + 0x71
#define OP_THROWERROR		OP_TYPEARGNONE + 0x72
#define OP_READBYTE			OP_TYPEARGNONE + 0x73
#define OP_WRITEBYTE		OP_TYPEARGNONE + 0x74
#define OP_STACKSWAP		OP_TYPEARGNONE + 0x7b
#define OP_STACKTOPTO2		OP_TYPEARGNONE + 0x7c
#define OP_STACKDUP			OP_TYPEARGNONE + 0x7d
#define OP_STACKDUP2		OP_TYPEARGNONE + 0x7e
#define OP_STACKSWAP2		OP_TYPEARGNONE + 0x7f

//Int argument ops GE OP_TYPEARGINT and LT OP_TYPEARG2INT
// RANGE 0x80 to 0xBF
#define OP_TYPEARGINT		0x80
#define OP_GOTO          	OP_TYPEARGINT + 0x01
#define OP_GOSUB         	OP_TYPEARGINT + 0x02
#define OP_BRANCH        	OP_TYPEARGINT + 0x03
#define OP_NUMASSIGN     	OP_TYPEARGINT + 0x04
#define OP_STRINGASSIGN  	OP_TYPEARGINT + 0x05
#define OP_ARRAYASSIGN   	OP_TYPEARGINT + 0x06
#define OP_STRARRAYASSIGN	OP_TYPEARGINT + 0x07
#define OP_PUSHVAR       	OP_TYPEARGINT + 0x08
#define OP_PUSHINT       	OP_TYPEARGINT + 0x09
#define OP_DEREF         	OP_TYPEARGINT + 0x0a
#define OP_FOR           	OP_TYPEARGINT + 0x0b
#define OP_NEXT          	OP_TYPEARGINT + 0x0c
#define OP_CURRLINE      	OP_TYPEARGINT + 0x0d
#define OP_DIM           	OP_TYPEARGINT + 0x0e
#define OP_DIMSTR        	OP_TYPEARGINT + 0x0f
#define OP_ONERROR         	OP_TYPEARGINT + 0x10
#define OP_EXPLODESTR		OP_TYPEARGINT + 0x11
#define OP_EXPLODESTR_C		OP_TYPEARGINT + 0x12
#define OP_EXPLODE		OP_TYPEARGINT + 0x13
#define OP_EXPLODE_C		OP_TYPEARGINT + 0x14
#define OP_EXPLODEXSTR		OP_TYPEARGINT + 0x15
#define OP_EXPLODEX		OP_TYPEARGINT + 0x16
#define OP_IMPLODE		OP_TYPEARGINT + 0x17
#define OP_GLOBAL		OP_TYPEARGINT + 0x18
#define OP_STAMP		OP_TYPEARGINT + 0x19	// stamp with 4 numbers x,y,scale,rotation and an array
#define OP_STAMP_LIST		OP_TYPEARGINT + 0x1a	// stamp with x, y, and an immediate list
#define OP_STAMP_S_LIST		OP_TYPEARGINT + 0x1b	// stamp with x, y, scale and an immediate list
#define OP_STAMP_SR_LIST	OP_TYPEARGINT + 0x1c	// stamp with x, y, scale, and rotation and an immediate list
#define OP_POLY_LIST		OP_TYPEARGINT + 0x1d
#define OP_WRITELINE		OP_TYPEARGINT + 0x1e
#define OP_ARRAYASSIGN2D   	OP_TYPEARGINT + 0x1f
#define OP_STRARRAYASSIGN2D	OP_TYPEARGINT + 0x20
#define OP_SOUND_ARRAY		OP_TYPEARGINT + 0x21
#define OP_SOUND_LIST		OP_TYPEARGINT + 0x22
#define OP_DEREF2D		OP_TYPEARGINT + 0x23
#define OP_REDIM          	OP_TYPEARGINT + 0x24
#define OP_REDIMSTR        	OP_TYPEARGINT + 0x25
#define OP_REDIM2D          	OP_TYPEARGINT + 0x26
#define OP_REDIMSTR2D        	OP_TYPEARGINT + 0x27
#define OP_ALEN 	       	OP_TYPEARGINT + 0x28
#define OP_ALENX 	       	OP_TYPEARGINT + 0x29
#define OP_ALENY 	       	OP_TYPEARGINT + 0x2a
#define OP_PUSHVARREF       	OP_TYPEARGINT + 0x2b	// push a T_VARREF numeric variable reference on stack
#define OP_PUSHVARREFSTR       	OP_TYPEARGINT + 0x2c	// push a T_VARREFSTR string variable reference on stack
#define OP_VARREFASSIGN       	OP_TYPEARGINT + 0x2d
#define OP_VARREFSTRASSIGN     	OP_TYPEARGINT + 0x2e
#define OP_FUNCRETURN       	OP_TYPEARGINT + 0x2f	// same as PUSHVAR with different error



//2 Int argument ops GE OP_TYPEARG2INT and LT OP_TYPEARGFLOAT
// RANGE 0xC0 to 0xCF
#define OP_TYPEARG2INT		0xC0
#define OP_ARRAYLISTASSIGN	OP_TYPEARG2INT + 0x00
#define OP_STRARRAYLISTASSIGN	OP_TYPEARG2INT + 0x01

//Float argument ops GE OP_TYPEARGFLOAT and LT OP_TYPEARGSTRING
// RANGE 0xD0 to 0xDF
#define OP_TYPEARGFLOAT		0xD0
#define OP_PUSHFLOAT		OP_TYPEARGFLOAT + 0x00

//String argument ops GE OP_TYPEARGSTRING and LT OP_TYPEARGEXT
// RANGE 0xE0 to 0xEF
#define OP_TYPEARGSTRING	0xE0
#define OP_PUSHSTRING		OP_TYPEARGSTRING + 0x00


// extended codes ops GE OP_TYPEARGEXT
#define OP_TYPEARGEXT		0xF0
#define OP_EXTENDEDNONE		OP_TYPEARGEXT + 0x00		// simple ops with no arguments
#define OP_EXTENDEDINT		OP_TYPEARGEXT + 0x01		// ops with one integer argument


// extended opcodes (second byte) - NO ARGMENTS
// first group OP_ENTENDED_NONE
#define OPX_SPRITEDIM 	       	0x00
#define OPX_SPRITELOAD 	       	0x01
#define OPX_SPRITESLICE 	       	0x02
#define OPX_SPRITEMOVE 	       	0x03
#define OPX_SPRITEHIDE 	       	0x04
#define OPX_SPRITESHOW 	       	0x05
#define OPX_SPRITECOLLIDE	0x06
#define OPX_SPRITEPLACE 	       	0x07
#define OPX_SPRITEX 	       	0x08
#define OPX_SPRITEY 	       	0x09
#define OPX_SPRITEH 	       	0x0a
#define OPX_SPRITEW 	       	0x0b
#define OPX_SPRITEV	       	0x0c
#define OPX_CHANGEDIR	       	0x0d
#define OPX_CURRENTDIR	       	0x0e
#define OPX_WAVWAIT		0x0f
// 0x10 unused
#define OPX_DBOPEN		0x11
#define OPX_DBCLOSE		0x12
#define OPX_DBEXECUTE		0x13
#define OPX_DBOPENSET		0x14
#define OPX_DBCLOSESET		0x15
#define OPX_DBROW		0x16
#define OPX_DBINT		0x17
#define OPX_DBFLOAT		0x18
#define OPX_DBSTRING		0x19
#define OPX_LASTERROR		0x1a
#define OPX_LASTERRORLINE	0x1b
#define OPX_LASTERRORMESSAGE	0x1c
#define OPX_LASTERROREXTRA	0x1d
#define OPX_OFFERROR		0x1e
#define OPX_NETLISTEN		0x1f
#define OPX_NETCONNECT		0x20
#define OPX_NETREAD		0x21
#define OPX_NETWRITE		0x22
#define OPX_NETCLOSE		0x23
#define OPX_NETDATA		0x24
#define OPX_NETADDRESS		0x25
#define OPX_KILL			0x26
#define OPX_MD5			0x27
#define OPX_SETSETTING		0x28
#define OPX_GETSETTING		0x29
#define OPX_PORTIN		0x2a
#define OPX_PORTOUT		0x2b
#define OPX_BINARYOR		0x2c
#define OPX_BINARYAND		0x2d
#define OPX_BINARYNOT		0x2e
#define OPX_IMGSAVE		0x2f
#define OPX_DIR			0x30
#define OPX_REPLACE		0x31
#define OPX_REPLACE_C		0x32
#define OPX_REPLACEX		0x33
#define OPX_COUNT		0x34
#define OPX_COUNT_C		0x35
#define OPX_COUNTX		0x36
#define OPX_OSTYPE		0x37
#define OPX_MSEC			0x38
#define OPX_EDITVISIBLE		0x39
#define OPX_GRAPHVISIBLE		0x3a
#define OPX_OUTPUTVISIBLE	0x3b
#define OPX_TEXTWIDTH		0x3c
#define OPX_SPRITER	       	0x3d
#define OPX_SPRITES	       	0x3e
#define OPX_FREEFILE	       	0x3f
#define OPX_FREENET	       	0x40
#define OPX_FREEDB	       	0x41
#define OPX_FREEDBSET	       	0x42
#define OPX_DBINTS		0x43
#define OPX_DBFLOATS		0x44
#define OPX_DBSTRINGS		0x45
#define OPX_DBNULL		0x46
#define OPX_DBNULLS		0x47
#define OPX_ARC			0x48
#define OPX_CHORD		0x49
#define OPX_PIE			0x4a
#define OPX_PENWIDTH			0x4b
#define OPX_GETPENWIDTH			0x4c
#define OPX_GETBRUSHCOLOR			0x4d
#define OPX_RUNTIMEWARNING	0x4e
#define OPX_ALERT	0x4f
#define OPX_CONFIRM	0x50
#define OPX_PROMPT	0x51
#define OPX_FROMRADIX	0x52
#define OPX_TORADIX	0x53
#define OPX_DEBUGINFO	0x54



