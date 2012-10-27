/** Copyright (C) 2012, J.M.Reneau.
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


// compile errors - returned by basicParse

#define COMPERR_SYNTAX	-1
#define COMPERR_SYNTAX_MESSAGE "Syntax error on line "
#define COMPERR_ENDIF	-2
#define COMPERR_ENDIF_MESSAGE "END IF without matching IF on line "
#define COMPERR_ELSE	-3
#define COMPERR_ELSE_MESSAGE "ELSE without matching IF on line "
#define COMPERR_ENDWHILE	-4
#define COMPERR_ENDWHILE_MESSAGE "END WHILE without matching WHILE on line "
#define COMPERR_UNTIL	-5
#define COMPERR_UNTIL_MESSAGE "UNTIL without matching DO on line "
#define COMPERR_NEXT	-6
#define COMPERR_NEXT_MESSAGE "NEXT without matching FOR on line "
#define COMPERR_IFNOEND	-7
#define COMPERR_IFNOEND_MESSAGE "IF without matching END IF or ELSE statement on line "
#define COMPERR_ELSENOEND	-8
#define COMPERR_ELSENOEND_MESSAGE "ELSE without matching END IF statement on line "
#define COMPERR_WHILENOEND	-9
#define COMPERR_WHILENOEND_MESSAGE "WHILE without matching END WHILE statement on line "
#define COMPERR_DONOEND	-10
#define COMPERR_DONOEND_MESSAGE "DO without matching UNTIL statement on line "
#define COMPERR_FORNOEND	-11
#define COMPERR_FORNOEND_MESSAGE "FOR without matching NEXT statement on line "
#define COMPERR_FUNCTIONNOEND	-12
#define COMPERR_FUNCTIONNOEND_MESSAGE "FUNCTION/SUBROUTINE without matching END FUNCTION/SUBROUTINE statement on line "
#define COMPERR_ENDFUNCTION	-13
#define COMPERR_ENDFUNCTION_MESSAGE "END FUNCTION/SUBROUTINE without matching FUNCTION/SUBROUTINE on line "
#define COMPERR_FUNCTIONNOTHERE	-14
#define COMPERR_FUNCTIONNOTHERE_MESSAGE "You may not define a FUNCTION/SUBROUTINE inside an IF, loop, or other FUNCTION/SUBROUTINE on line "
#define COMPERR_GLOBALNOTHERE	-15
#define COMPERR_GLOBALNOTHERE_MESSAGE "You may not define GLOBAL variable(s) inside an IF, loop, or FUNCTION/SUBROUTINE on line "
#define COMPERR_FUNCTIONGOTO	-16
#define COMPERR_FUNCTIONGOTO_MESSAGE "You may not define a label or use a GOTO or GOSUB statement in a FUNCTION/SUBROUTINE declaration on line "
#define COMPERR_ASSIGNN2S	-17
#define COMPERR_ASSIGNN2S_MESSAGE "Error assigning a number to a string variable on line "
#define COMPERR_ASSIGNS2N	-18
#define COMPERR_ASSIGNS2N_MESSAGE "Error assigning a string to a numeric variable on line "


