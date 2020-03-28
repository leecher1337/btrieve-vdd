#
# (C) Copyright 1992, ..., 2007 the "DOSEMU-Development-Team".
#
# for details see file COPYING.DOSEMU in the DOSEMU distribution
#

top_builddir=../../..
include $(top_builddir)/Makefile.conf

D:=$(BINPATH)/commands

CFILES = btrfnc.c btrfnc_linux.c mod_dosemu.c
HFILES = btrfnc.h
SYS    = $(D)/btrdrvr.sys
DEPENDS= $(CFILES:.c=.d)
OBJS   = $(CFILES:.c=.o)
CPPFLAGS += -DBTI_HPUX=1
ALL_CFLAGS += -DBTI_HPUX=1

ALL = $(CFILES) $(HFILES)

$(D)/%.sys : dev/dosemu/%.sys
	cp $< $@

all: lib $(SYS)

install:

include $(REALTOPDIR)/src/Makefile.common
