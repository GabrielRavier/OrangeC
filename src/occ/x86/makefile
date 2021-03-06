# Software License Agreement
# 
#     Copyright(C) 1994-2018 David Lindauer, (LADSoft)
# 
#     This file is part of the Orange C Compiler package.
# 
#     The Orange C Compiler package is free software: you can redistribute it and/or modify
#     it under the terms of the GNU General Public License as published by
#     the Free Software Foundation, either version 3 of the License, or
#     (at your option) any later version, with the addition of the 
#     Orange C "Target Code" exception.
# 
#     The Orange C Compiler package is distributed in the hope that it will be useful,
#     but WITHOUT ANY WARRANTY; without even the implied warranty of
#     MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#     GNU General Public License for more details.
# 
#     You should have received a copy of the GNU General Public License
#     along with Orange C.  If not, see <http://www.gnu.org/licenses/>.
# 
#     contact information:
#         email: TouchStone222@runbox.com <David Lindauer>
# 

include ../../pathext2.mak

NAME=occ
MAIN_FILE=ccmain.cpp
C_DEPENDENCIES= \
    winsystem.c

CPP_DEPENDENCIES= \
    ccmain.cpp \
    config.cpp \
    crc32.cpp \
    beinterf.cpp \
    browse.cpp \
    ccerr.cpp \
    constopt.cpp \
    cppbltin.cpp \
    cpplookup.cpp \
    debug.cpp \
    declare.cpp \
    declcons.cpp \
    declcpp.cpp \
    expr.cpp \
    exprcpp.cpp \
    float.cpp \
    help.cpp \
    ialias.cpp \
    iblock.cpp \
    iconfl.cpp \
    iconst.cpp \
    iexpr.cpp \
    iflow.cpp \
    iinline.cpp \
    iinvar.cpp \
    ilazy.cpp \
    ilive.cpp \
    ilocal.cpp \
    iloop.cpp \
    init.cpp \
    inline.cpp \
    invoke.cpp \
    ioptutil.cpp \
    iout.cpp \
    ipeep.cpp \
    irc.cpp \
    ireshape.cpp \
    irewrite.cpp \
    issa.cpp \
    istmt.cpp \
    istren.cpp \
    lambda.cpp \
    lex.cpp \
    libcxx.cpp \
    list.cpp \
    mangle.cpp \
    memory.cpp \
    osutil.cpp \
    output.cpp \
    preproc.cpp \
    property.cpp \
    ppexpr.cpp \
    rewrite.cpp \
    rtti.cpp \
    stmt.cpp \
    symtab.cpp \
    template.cpp \
    types.cpp \
    unmangle.cpp \
    wseh.cpp \
    beIntrins.cpp \
    dbgtypes.cpp \
    gen.cpp \
    outasm.cpp \
    outcode.cpp \
    inasm.cpp \
    peep.cpp \
    InstructionParser.cpp \
    x64stub.cpp \
    x64Parser.cpp

ifeq "$(COMPILER)" "gcc-linux"
H_FILES=$(wildcard *.h) $(wildcard ../parse/*.h) $(wildcard ../middle/*.h) $(wildcard ../preproc/*.h)
else
H_FILES=$(wildcard *.h) $(wildcard ..$(PATHEXT2)parse$(PATHEXT2)*.h) $(wildcard ..$(PATHEXT2)middle$(PATHEXT2)*.h) $(wildcard ..$(PATHEXT2)preproc$(PATHEXT2)*.h)
endif

LIB_DEPENDENCIES=oasm ocpplib objlib util
#determine if we can ignore the banner for a batch compile
#use long long type
LL = USE_LONGLONG
DEFINES=$(i386) $(LL) $(MSDOS) x64

ifeq "$(COMPILER)" "gcc-linux"
SOURCE = ..$(PATHEXT2)parse:..$(PATHEXT2)preproc:..$(PATHEXT2)middle
INCLUDES=..$(PATHEXT2)parse ..$(PATHEXT2)preproc ..$(PATHEXT2)middle ..$(PATHEXT2)..$(PATHEXT2)ocpp ..$(PATHEXT2)..$(PATHEXT2)oasm ..$(PATHEXT2)..$(PATHEXT2)util ..$(PATHEXT2)..$(PATHEXT2)objlib
SRCPATH=.$(PATHEXT2):$(SOURCE):..:..$(PATHEXT2)..$(PATHEXT2)util:..$(PATHEXT2)
else
SOURCE = ..$(PATHEXT2)parse$(PATHEXT2);..$(PATHEXT2)preproc$(PATHEXT2);..$(PATHEXT2)middle$(PATHEXT2)
INCLUDES=..$(PATHEXT2)parse ..$(PATHEXT2)preproc ..$(PATHEXT2)middle ..$(PATHEXT2)..$(PATHEXT2)ocpp ..$(PATHEXT2)..$(PATHEXT2)oasm ..$(PATHEXT2)..$(PATHEXT2)util ..$(PATHEXT2)..$(PATHEXT2)objlib
SRCPATH=.$(PATHEXT2);$(SOURCE);..;..$(PATHEXT2)..$(PATHEXT2)util$(PATHEXT2);..$(PATHEXT2)
ifeq "$(ISOMAKE)" ""
SRCPATH:=$(subst $(PATHEXT2),/,$(SRCPATH))
endif
endif

vpath %.c $(SRCPATH)
vpath %.cpp $(SRCPATH)

ifndef _TREEROOT
_TREEROOT := $(CURDIR)$(PATHEXT2)..$(PATHEXT2)..$(PATHEXT2)treetop.mak
_TREETARGET := $(CURDIR)
export _TREEROOT
export _TREETARGET

redirect:
	@$(MAKE) -f$(_TREEROOT)
format:
	@$(MAKE) -f $(_TREEROOT) format
endif

ifeq "$(COMPILER)" "gcc-linux"
x64Parser.cpp: ..$(PATHEXT2)..$(PATHEXT2)oasm$(PATHEXT2)x64Parser.cpp
	cp $< $@
else
x64Parser.cpp: ..$(PATHEXT2)..$(PATHEXT2)oasm$(PATHEXT2)x64Parser.cpp
	copy $< $@
endif

DISTRIBUTE: copyexe
	copy target.cfg $(DISTBIN)$(PATHEXT2)occ.cfg