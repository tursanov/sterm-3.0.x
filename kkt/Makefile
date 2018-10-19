include Makefile.conf
DIR = kkt/

INCLUDE = ../include

KKT_OBJS =		\
	io		\
	kkt		\
	parser

OBJS = $(addsuffix .o, $(KKT_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
