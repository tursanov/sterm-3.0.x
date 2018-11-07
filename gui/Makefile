include Makefile.conf
DIR = gui/

INCLUDE = ../include

GUI_OBJS =		\
	adv_calc	\
	calc		\
	cheque		\
	dialog		\
	exgdi		\
	fa		\
	forms		\
	gdi		\
	help		\
	menu		\
	options		\
	ping		\
	scr		\
	ssaver		\
	status		\
	xchange

OBJS = $(addsuffix .o, $(GUI_OBJS))

LOG = log

.PHONY:	$(LOG)

all:		depend $(OBJS) $(LOG)

$(LOG):
		@$(MAKE) -C $@ -I../../

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend
		@$(MAKE) depend -C $(LOG) -I../../

clean:
		@rm -f $(BIN_GARBAGE)
		@$(MAKE) clean -C log -I../../

ifneq ($(wildcard .depend),)
include .depend
endif
