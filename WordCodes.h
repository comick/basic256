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

// BE SURE TO ADD NEW OP TO ENUM

// ALSO ADD to the optype, opname, and opxname functions in interperter.c

#define OPTYPE_NONE				0x00000000		// simple opcode with no inline data
#define OPTYPE_INT				0x01000000		// a trailing integer
#define OPTYPE_FLOAT			0x02000000		// decimal number following
#define OPTYPE_STRING			0x03000000		// null terminated string following
#define OPTYPE_LABEL			0x04000000		// label number (int) - converted to address at runtime
#define OPTYPE_VARIABLE			0x05000000		// variable number (int)
#define	OPTYPE_MASK				0xff000000		// and mask to strip optype out of opcode


#define OP_END					OPTYPE_NONE + 0x000000
#define OP_NOP 					OPTYPE_NONE + 0x000001
#define OP_RETURN 				OPTYPE_NONE + 0x000002
#define OP_EQUAL 				OPTYPE_NONE + 0x000004
#define OP_NEQUAL 				OPTYPE_NONE + 0x000005
#define OP_GT 					OPTYPE_NONE + 0x000006
#define OP_LT 					OPTYPE_NONE + 0x000007
#define OP_GTE 					OPTYPE_NONE + 0x000008   
#define OP_LTE 					OPTYPE_NONE + 0x000009   
#define OP_AND 					OPTYPE_NONE + 0x00000a   
#define OP_NOT 					OPTYPE_NONE + 0x00000b   
#define OP_OR 					OPTYPE_NONE + 0x00000c   
#define OP_XOR 					OPTYPE_NONE + 0x00000d   
#define OP_INT 					OPTYPE_NONE + 0x00000e   
#define OP_STRING 				OPTYPE_NONE + 0x00000f
#define OP_ADD 					OPTYPE_NONE + 0x000010
#define OP_SUB 					OPTYPE_NONE + 0x000011
#define OP_MUL 					OPTYPE_NONE + 0x000012
#define OP_DIV 					OPTYPE_NONE + 0x000013
#define OP_EX 					OPTYPE_NONE + 0x000014
#define OP_NEGATE 				OPTYPE_NONE + 0x000015
#define OP_PRINT 				OPTYPE_NONE + 0x000016
#define OP_PRINTN 				OPTYPE_NONE + 0x000017
#define OP_INPUT 				OPTYPE_NONE + 0x000018
#define OP_KEY 					OPTYPE_NONE + 0x000019
#define OP_PLOT 				OPTYPE_NONE + 0x00001a
#define OP_RECT 				OPTYPE_NONE + 0x00001b
#define OP_CIRCLE 				OPTYPE_NONE + 0x00001c
#define OP_LINE 				OPTYPE_NONE + 0x00001d
#define OP_REFRESH 				OPTYPE_NONE + 0x00001e
#define OP_FASTGRAPHICS			OPTYPE_NONE + 0x00001f
#define OP_CLS 					OPTYPE_NONE + 0x000020
#define OP_CLG 					OPTYPE_NONE + 0x000021
#define OP_GRAPHSIZE 			OPTYPE_NONE + 0x000022
#define OP_GRAPHWIDTH 			OPTYPE_NONE + 0x000023
#define OP_GRAPHHEIGHT 			OPTYPE_NONE + 0x000024
#define OP_SIN 					OPTYPE_NONE + 0x000025
#define OP_COS 					OPTYPE_NONE + 0x000026
#define OP_TAN 					OPTYPE_NONE + 0x000027
#define OP_RAND 				OPTYPE_NONE + 0x000028
#define OP_CEIL 				OPTYPE_NONE + 0x000029
#define OP_FLOOR 				OPTYPE_NONE + 0x00002a
#define OP_ABS 					OPTYPE_NONE + 0x00002b
#define OP_PAUSE 				OPTYPE_NONE + 0x00002c
#define OP_LENGTH 				OPTYPE_NONE + 0x00002d
#define OP_MID 					OPTYPE_NONE + 0x00002e
#define OP_INSTR 				OPTYPE_NONE + 0x00002f
#define OP_INSTRX 				OPTYPE_NONE + 0x000032
#define OP_OPEN 				OPTYPE_NONE + 0x000034
#define OP_READ 				OPTYPE_NONE + 0x000035
#define OP_WRITE 				OPTYPE_NONE + 0x000036
#define OP_CLOSE 				OPTYPE_NONE + 0x000037
#define OP_RESET 				OPTYPE_NONE + 0x000038
#define OP_INCREASERECURSE		OPTYPE_NONE + 0x000039
#define OP_DECREASERECURSE		OPTYPE_NONE + 0x00003a
#define OP_ASC 					OPTYPE_NONE + 0x00003b
#define OP_CHR 					OPTYPE_NONE + 0x00003c
#define OP_FLOAT 				OPTYPE_NONE + 0x00003d
#define OP_READLINE 			OPTYPE_NONE + 0x00003e
#define OP_EOF 					OPTYPE_NONE + 0x00003f
#define OP_MOD 					OPTYPE_NONE + 0x000040
#define OP_YEAR 				OPTYPE_NONE + 0x000041
#define OP_MONTH 				OPTYPE_NONE + 0x000042
#define OP_DAY 					OPTYPE_NONE + 0x000043
#define OP_HOUR 				OPTYPE_NONE + 0x000044
#define OP_MINUTE 				OPTYPE_NONE + 0x000045
#define OP_SECOND 				OPTYPE_NONE + 0x000046
#define OP_MOUSEX 				OPTYPE_NONE + 0x000047
#define OP_MOUSEY 				OPTYPE_NONE + 0x000048
#define OP_MOUSEB 				OPTYPE_NONE + 0x000049
#define OP_CLICKCLEAR 			OPTYPE_NONE + 0x00004a
#define OP_CLICKX 				OPTYPE_NONE + 0x00004b
#define OP_CLICKY 				OPTYPE_NONE + 0x00004c
#define OP_CLICKB 				OPTYPE_NONE + 0x00004d
#define OP_TEXT 				OPTYPE_NONE + 0x00004e
#define OP_FONT 				OPTYPE_NONE + 0x00004f
#define OP_SAY 					OPTYPE_NONE + 0x000050
#define OP_WAVPLAY 				OPTYPE_NONE + 0x000051
#define OP_WAVSTOP 				OPTYPE_NONE + 0x000052
#define OP_SEEK 				OPTYPE_NONE + 0x000053
#define OP_SIZE 				OPTYPE_NONE + 0x000054
#define OP_EXISTS 				OPTYPE_NONE + 0x000055
#define OP_LEFT 				OPTYPE_NONE + 0x000056
#define OP_RIGHT 				OPTYPE_NONE + 0x000057
#define OP_UPPER 				OPTYPE_NONE + 0x000058
#define OP_LOWER 				OPTYPE_NONE + 0x000059
#define OP_SYSTEM 				OPTYPE_NONE + 0x00005a
#define OP_VOLUME 				OPTYPE_NONE + 0x00005b
#define OP_SETCOLOR 			OPTYPE_NONE + 0x00005c
#define OP_RGB 					OPTYPE_NONE + 0x00005d
#define OP_PIXEL 				OPTYPE_NONE + 0x00005e
#define OP_GETCOLOR 			OPTYPE_NONE + 0x00005f
#define OP_ASIN 				OPTYPE_NONE + 0x000060
#define OP_ACOS 				OPTYPE_NONE + 0x000061
#define OP_ATAN 				OPTYPE_NONE + 0x000062
#define OP_DEGREES 				OPTYPE_NONE + 0x000063
#define OP_RADIANS 				OPTYPE_NONE + 0x000064
#define OP_INTDIV 				OPTYPE_NONE + 0x000065
#define OP_LOG 					OPTYPE_NONE + 0x000066
#define OP_LOGTEN 				OPTYPE_NONE + 0x000067
#define OP_GETSLICE 			OPTYPE_NONE + 0x000068
#define OP_PUTSLICE 			OPTYPE_NONE + 0x000069
#define OP_PUTSLICEMASK 		OPTYPE_NONE + 0x00006a
#define OP_IMGLOAD 				OPTYPE_NONE + 0x00006b
#define OP_SQR 					OPTYPE_NONE + 0x00006c
#define OP_EXP 					OPTYPE_NONE + 0x00006d
#define OP_ARGUMENTCOUNTTEST 	OPTYPE_NONE + 0x00006e
#define OP_THROWERROR 			OPTYPE_NONE + 0x00006f
#define OP_READBYTE 			OPTYPE_NONE + 0x000070
#define OP_WRITEBYTE 			OPTYPE_NONE + 0x000071
#define OP_STACKSWAP 			OPTYPE_NONE + 0x000072
#define OP_STACKTOPTO2 			OPTYPE_NONE + 0x000073
#define OP_STACKDUP 			OPTYPE_NONE + 0x000074
#define OP_STACKDUP2 			OPTYPE_NONE + 0x000075
#define OP_STACKSWAP2 			OPTYPE_NONE + 0x000076
#define OP_STAMP_LIST 			OPTYPE_NONE + 0x000077
#define OP_STAMP_S_LIST 		OPTYPE_NONE + 0x000078
#define OP_STAMP_SR_LIST 		OPTYPE_NONE + 0x000079
#define OP_POLY_LIST 			OPTYPE_NONE + 0x00007a
#define OP_WRITELINE 			OPTYPE_NONE + 0x00007b
#define OP_SOUND_LIST 			OPTYPE_NONE + 0x00007c
#define OP_SPRITEPOLY_LIST 		OPTYPE_NONE + 0x00007d
#define OP_SPRITEDIM			OPTYPE_NONE + 0x00007e
#define OP_SPRITELOAD			OPTYPE_NONE + 0x00007f
#define OP_SPRITESLICE			OPTYPE_NONE + 0x000080
#define OP_SPRITEMOVE			OPTYPE_NONE + 0x000081
#define OP_SPRITEHIDE			OPTYPE_NONE + 0x000082
#define OP_SPRITESHOW			OPTYPE_NONE + 0x000083
#define OP_SPRITECOLLIDE		OPTYPE_NONE + 0x000084
#define OP_SPRITEPLACE			OPTYPE_NONE + 0x000085
#define OP_SPRITEX				OPTYPE_NONE + 0x000086
#define OP_SPRITEY				OPTYPE_NONE + 0x000087
#define OP_SPRITEH				OPTYPE_NONE + 0x000088
#define OP_SPRITEW				OPTYPE_NONE + 0x000089
#define OP_SPRITEV				OPTYPE_NONE + 0x00008a
#define OP_CHANGEDIR			OPTYPE_NONE + 0x00008b
#define OP_CURRENTDIR			OPTYPE_NONE + 0x00008c
#define OP_WAVWAIT				OPTYPE_NONE + 0x00008d
#define OP_DBOPEN				OPTYPE_NONE + 0x00008e
#define OP_DBCLOSE				OPTYPE_NONE + 0x00008f
#define OP_DBEXECUTE			OPTYPE_NONE + 0x000090
#define OP_DBOPENSET			OPTYPE_NONE + 0x000091
#define OP_DBCLOSESET			OPTYPE_NONE + 0x000092
#define OP_DBROW				OPTYPE_NONE + 0x000093
#define OP_DBINT				OPTYPE_NONE + 0x000094
#define OP_DBFLOAT				OPTYPE_NONE + 0x000095
#define OP_DBSTRING				OPTYPE_NONE + 0x000096
#define OP_LASTERROR			OPTYPE_NONE + 0x000097
#define OP_LASTERRORLINE		OPTYPE_NONE + 0x000098
#define OP_LASTERRORMESSAGE		OPTYPE_NONE + 0x000099
#define OP_LASTERROREXTRA		OPTYPE_NONE + 0x00009a
#define OP_OFFERROR				OPTYPE_NONE + 0x00009b
#define OP_NETLISTEN			OPTYPE_NONE + 0x00009c
#define OP_NETCONNECT			OPTYPE_NONE + 0x00009d
#define OP_NETREAD				OPTYPE_NONE + 0x00009e
#define OP_NETWRITE				OPTYPE_NONE + 0x00009f
#define OP_NETCLOSE				OPTYPE_NONE + 0x0000a0
#define OP_NETDATA				OPTYPE_NONE + 0x0000a1
#define OP_NETADDRESS			OPTYPE_NONE + 0x0000a2
#define OP_KILL					OPTYPE_NONE + 0x0000a3
#define OP_MD5					OPTYPE_NONE + 0x0000a4
#define OP_SETSETTING			OPTYPE_NONE + 0x0000a5
#define OP_GETSETTING			OPTYPE_NONE + 0x0000a6
#define OP_PORTIN				OPTYPE_NONE + 0x0000a7
#define OP_PORTOUT				OPTYPE_NONE + 0x0000a8
#define OP_BINARYOR				OPTYPE_NONE + 0x0000a9
#define OP_BINARYAND			OPTYPE_NONE + 0x0000aa
#define OP_BINARYNOT			OPTYPE_NONE + 0x0000ab
#define OP_IMGSAVE				OPTYPE_NONE + 0x0000ac
#define OP_DIR					OPTYPE_NONE + 0x0000ad
#define OP_REPLACE				OPTYPE_NONE + 0x0000ae
#define OP_REPLACEX				OPTYPE_NONE + 0x0000b0
#define OP_COUNT				OPTYPE_NONE + 0x0000b1
#define OP_COUNTX				OPTYPE_NONE + 0x0000b3
#define OP_OSTYPE				OPTYPE_NONE + 0x0000b4
#define OP_MSEC					OPTYPE_NONE + 0x0000b5
#define OP_EDITVISIBLE			OPTYPE_NONE + 0x0000b6
#define OP_GRAPHVISIBLE			OPTYPE_NONE + 0x0000b7
#define OP_OUTPUTVISIBLE		OPTYPE_NONE + 0x0000b8
#define OP_TEXTWIDTH			OPTYPE_NONE + 0x0000b9
#define OP_TEXTHEIGHT			OPTYPE_NONE + 0x0000ba
#define OP_SPRITER				OPTYPE_NONE + 0x0000bb
#define OP_SPRITES				OPTYPE_NONE + 0x0000bc
#define OP_FREEFILE				OPTYPE_NONE + 0x0000bd
#define OP_FREENET				OPTYPE_NONE + 0x0000be
#define OP_FREEDB				OPTYPE_NONE + 0x0000bf
#define OP_FREEDBSET			OPTYPE_NONE + 0x0000c0
#define OP_DBNULL				OPTYPE_NONE + 0x0000c4
#define OP_DBNULLS				OPTYPE_NONE + 0x0000c5
#define OP_ARC					OPTYPE_NONE + 0x0000c6
#define OP_CHORD				OPTYPE_NONE + 0x0000c7
#define OP_PIE					OPTYPE_NONE + 0x0000c8
#define OP_PENWIDTH				OPTYPE_NONE + 0x0000c9
#define OP_GETPENWIDTH			OPTYPE_NONE + 0x0000ca
#define OP_GETBRUSHCOLOR		OPTYPE_NONE + 0x0000cb
#define OP_ALERT				OPTYPE_NONE + 0x0000cc
#define OP_CONFIRM				OPTYPE_NONE + 0x0000cd
#define OP_PROMPT				OPTYPE_NONE + 0x0000ce
#define OP_FROMRADIX			OPTYPE_NONE + 0x0000cf
#define OP_TORADIX				OPTYPE_NONE + 0x0000d0
#define OP_PRINTERON			OPTYPE_NONE + 0x0000d1
#define OP_PRINTEROFF			OPTYPE_NONE + 0x0000d2
#define OP_PRINTERPAGE			OPTYPE_NONE + 0x0000d3
#define OP_PRINTERCANCEL		OPTYPE_NONE + 0x0000d4
#define OP_DEBUGINFO			OPTYPE_NONE + 0x0000d5
#define OP_WAVLENGTH			OPTYPE_NONE + 0x0000d6
#define OP_WAVPOS				OPTYPE_NONE + 0x0000d7
#define OP_WAVPAUSE				OPTYPE_NONE + 0x0000d8
#define OP_WAVSEEK				OPTYPE_NONE + 0x0000d9
#define OP_WAVSTATE				OPTYPE_NONE + 0x0000da
#define OP_MIDX					OPTYPE_NONE + 0x0000db
#define OP_REGEXMINIMAL			OPTYPE_NONE + 0x0000dc
#define OP_OPENSERIAL			OPTYPE_NONE + 0x0000dd
#define OP_TYPEOF				OPTYPE_NONE + 0x0000de
#define OP_CONCATENATE			OPTYPE_NONE + 0x0000df
#define OP_ISNUMERIC			OPTYPE_NONE + 0x0000e0
#define OP_LTRIM				OPTYPE_NONE + 0x0000e1
#define OP_RTRIM				OPTYPE_NONE + 0x0000e2
#define OP_TRIM					OPTYPE_NONE + 0x0000e3
#define OP_KEYPRESSED			OPTYPE_NONE + 0x0000e4


#define OP_GOTO 				OPTYPE_LABEL + 0x000000
#define OP_GOSUB 				OPTYPE_LABEL + 0x000001
#define OP_BRANCH 				OPTYPE_LABEL + 0x000002
#define OP_ONERRORGOSUB 		OPTYPE_LABEL + 0x000003
#define OP_ONERRORCATCH 		OPTYPE_LABEL + 0x000004
#define OP_EXITFOR				OPTYPE_LABEL + 0x0000e5

#define OP_ASSIGN 				OPTYPE_VARIABLE + 0x000000
#define OP_ARRAYASSIGN 			OPTYPE_VARIABLE + 0x000002
#define OP_PUSHVAR 				OPTYPE_VARIABLE + 0x000004
#define OP_FOR 					OPTYPE_VARIABLE + 0x000006
#define OP_NEXT 				OPTYPE_VARIABLE + 0x000007
#define OP_DIM 					OPTYPE_VARIABLE + 0x000008
#define OP_DEREF 				OPTYPE_VARIABLE + 0x00000c
#define OP_REDIM 				OPTYPE_VARIABLE + 0x00000d
#define OP_ALEN 				OPTYPE_VARIABLE + 0x000011
#define OP_ALENX 				OPTYPE_VARIABLE + 0x000012
#define OP_ALENY 				OPTYPE_VARIABLE + 0x000013
#define OP_PUSHVARREF 			OPTYPE_VARIABLE + 0x000014
#define OP_VARREFASSIGN 		OPTYPE_VARIABLE + 0x000016
#define OP_ARRAY2STACK 			OPTYPE_VARIABLE + 0x000019
#define OP_EXPLODE				OPTYPE_VARIABLE + 0x00001e
#define OP_EXPLODEX				OPTYPE_VARIABLE + 0x000020
#define OP_GLOBAL				OPTYPE_VARIABLE + 0x000021
#define OP_IMPLODE				OPTYPE_VARIABLE + 0x000022
#define OP_UNASSIGN				OPTYPE_VARIABLE + 0x000023
#define OP_UNASSIGNA			OPTYPE_VARIABLE + 0x000024
#define OP_VARIABLEWATCH		OPTYPE_VARIABLE + 0x000025



#define OP_PUSHINT 				OPTYPE_INT + 0x000000
#define OP_CURRLINE 			OPTYPE_INT + 0x000001
#define OP_ARRAYLISTASSIGN 		OPTYPE_INT + 0x000002

#define OP_PUSHFLOAT 			OPTYPE_FLOAT + 0x000000

#define OP_PUSHSTRING 			OPTYPE_STRING + 0x000000
#define OP_INCLUDEFILE 			OPTYPE_STRING + 0x000001








