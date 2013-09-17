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
#define COMPERR_ENDIF	-2
#define COMPERR_ELSE	-3
#define COMPERR_ENDWHILE	-4
#define COMPERR_UNTIL	-5
#define COMPERR_NEXT	-6
#define COMPERR_IFNOEND	-7
#define COMPERR_ELSENOEND	-8
#define COMPERR_WHILENOEND	-9
#define COMPERR_DONOEND	-10
#define COMPERR_FORNOEND	-11
#define COMPERR_FUNCTIONNOEND	-12
#define COMPERR_ENDFUNCTION	-13
#define COMPERR_FUNCTIONNOTHERE	-14
#define COMPERR_GLOBALNOTHERE	-15
#define COMPERR_FUNCTIONGOTO	-16
#define COMPERR_ASSIGNN2S	-17
#define COMPERR_ASSIGNS2N	-18
#define COMPERR_RETURNVALUE	-19
#define COMPERR_RETURNTYPE	-20
#define COMPERR_CONTINUEDO	-21
#define COMPERR_CONTINUEFOR	-22
#define COMPERR_CONTINUEWHILE	-23
#define COMPERR_EXITDO	-24
#define COMPERR_EXITFOR	-25
#define COMPERR_EXITWHILE	-26
#define COMPERR_INCLUDEFILE	-27
#define COMPERR_INCLUDEDEPTH	-28
