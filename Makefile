MAKEFLAGS += -rR --no-print-directory
ifneq ($(wildcard Makefile.conf),)
include Makefile.conf
endif
include Makefile.rules

BASE_OBJS =		\
	base64		\
	cfg		\
	config		\
	devinfo		\
	ds1990a		\
	express		\
	gd		\
	genfunc		\
	hash		\
	hex		\
	iplir		\
	kbd		\
	keys		\
	md5		\
	rndtbl		\
	serial		\
	sterm		\
	tki		\
	transport	\
	xchange

ifdef __USE_ISRV__
	BASE_OBJS += imsg
endif

GUI_OBJS =		\
	adv_calc	\
	calc		\
	dialog		\
	exgdi		\
	gdi		\
	help		\
	menu		\
	options		\
	ping		\
	scr		\
	ssaver		\
	status		\
	xchange

GUI_LOG_OBJS =		\
	express		\
	generic		\
	local		\
	pos

KKT_OBJS =		\
	io		\
	kkt		\
	parser

LOG_OBJS =		\
	express		\
	generic		\
	local		\
	pos

POS_OBJS =		\
	command		\
	error		\
	pos		\
	printer		\
	screen		\
	serial		\
	tcp

PPP_OBJS =		\
	ppp

PRN_OBJS =		\
	aux		\
	express		\
	generic		\
	local

USB_OBJS =		\
	core		\
	key

OBJS =			\
	$(addprefix base/,	$(addsuffix .o, $(BASE_OBJS)))		\
	$(addprefix kkt/,	$(addsuffix .o, $(KKT_OBJS)))		\
	$(addprefix log/,	$(addsuffix .o, $(LOG_OBJS)))		\
	$(addprefix gui/,	$(addsuffix .o, $(GUI_OBJS)))		\
	$(addprefix gui/log/,	$(addsuffix .o, $(GUI_LOG_OBJS)))	\
	$(addprefix pos/,	$(addsuffix .o, $(POS_OBJS)))		\
	$(addprefix ppp/,	$(addsuffix .o, $(PPP_OBJS)))		\
	$(addprefix prn/,	$(addsuffix .o, $(PRN_OBJS)))		\
	$(addprefix usb/,	$(addsuffix .o, $(USB_OBJS)))

SUBDIRS =		\
	base		\
	gui		\
	kkt		\
	log		\
	pos		\
	ppp		\
	prn		\
	usb		\
	modules		\
	helpers		\
	scripts

.PHONY:	$(SUBDIRS)

all:	inc_build $(SUBDIRS) sterm mk_env

inc_build:
	@./helpers/setbuild.pl

$(SUBDIRS):
	@$(MAKE) -C $@ -I../

sterm:	$(OBJS)
	@echo -e "\t$(LD_NAME)   $@"
	@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

mk_env:
	@if [ ! -f $(STERM_HOME)/sterm.dat ]; then\
		./helpers/mw --write-tki=$(STERM_HOME)/sterm.dat --number --chipset=cardless;\
	fi
	@./helpers/mw --read-tki=$(STERM_HOME)/sterm.dat --all

clean:
	@rm -f sterm
	@for dir in $(SUBDIRS); do \
		$(MAKE) clean -C $$dir -I../; \
	done

more_clean:
	@rm -f	Makefile.conf \
		include/config.h \
		base/config.c
	@find . -name .depend -delete
	@find resp -name "*.rsp" -delete

dist_clean:	clean more_clean

CFG_DIR = "cfg"
RELEASE_CONFIG = $(CFG_DIR)/release
DEBUG_CONFIG = $(CFG_DIR)/debug

release-config:
	@helpers/configure.pl $(RELEASE_CONFIG)

debug-config:
	@helpers/configure.pl $(DEBUG_CONFIG)
