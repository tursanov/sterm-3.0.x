include Makefile.conf
DIR = fd/

INCLUDE = ../../include

FFD_OBJS =	\
	tlv		\
	ad 		\
	fd		\

OBJS = $(addsuffix .o, $(FFD_OBJS))

all:		depend $(OBJS)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
