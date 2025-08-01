include Makefile.conf
DIR = scripts/

INCLUDE = ../include

BINARIES =		\
	bnklic		\
	dsd		\
	iplirhlp	\
	lprnlic		\
	multiport	\
	mwl		\
	rmbnklic	\
	rmbnklicx	\
	rmlprnlic	\
	rmlprnlicx	\
	tpld		\
	tstbeep		\
	tstcom		\
	unlkbnk		\
	unlklprn	\
	usbcheck	\
	usbread

BASE_OBJS =		\
	base64		\
	cfg		\
	ds1990a		\
	genfunc		\
	hex		\
	licsig		\
	md5		\
	rndtbl		\
	serial		\
	tki

LOG_OBJS =		\
	generic		\
	express		\
	logdbg

SCRIPTS_OBJS =		\
	bnklic		\
	dsd		\
	iplirhlp	\
	lprnlic		\
	multiport	\
	mwl		\
	rmlic		\
	rmbnklic	\
	rmbnklicx	\
	rmlprnlic	\
	rmlprnlicx	\
	tpld		\
	tstbeep		\
	tstcom		\
	unlkbnk		\
	unlklprn	\
	unlock		\
	usbcheck	\
	usbread

USB_OBJ =		\
	core		\
	key

DIR_BASE = ../base/
DIR_LOG = ../log/
DIR_USB = ../usb/

OBJS =			\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(BASE_OBJS))) \
	$(addprefix $(DIR_LOG), $(addsuffix .o, $(LOG_OBJS))) \
	$(addsuffix .o, $(SCRIPTS_OBJS)) \
	$(addprefix $(DIR_USB), $(addsuffix .o, $(USB_OBJS)))

BIN_GARBAGE += $(BINARIES)

all:		depend $(BINARIES)

depend:
		@$(CC) $(CFLAGS) -MM $(OBJS:.o=.c) > .depend

## bnklic
BNKLIC_BASE_OBJS =	\
	licsig		\
	md5

BNKLIC_SCRIPTS_OBJS =	\
	bnklic

BNKLIC_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(BNKLIC_BASE_OBJS))) \
	$(addsuffix .o, $(BNKLIC_SCRIPTS_OBJS))

bnklic:		$(BNKLIC_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## dsd
DSD_BASE_OBJS =		\
	serial

DSD_SCRIPTS_OBJS =	\
	dsd

DSD_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(DSD_BASE_OBJS))) \
	$(addsuffix .o, $(DSD_SCRIPTS_OBJS))

dsd:		$(DSD_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## iplirhlp
IPLIRHLP_BASE_OBJS =	\
	md5

IPLIRHLP_SCRIPTS_OBJS =	\
	iplirhlp

IPLIRHLP_USB_OBJS =	\
	core		\
	key

IPLIRHLP_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(IPLIRHLP_BASE_OBJS))) \
	$(addsuffix .o, $(IPLIRHLP_SCRIPTS_OBJS)) \
	$(addprefix $(DIR_USB), $(addsuffix .o, $(IPLIRHLP_USB_OBJS)))

iplirhlp:	$(IPLIRHLP_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## lprnlic
LPRNLIC_BASE_OBJS =	\
	licsig		\
	md5

LPRNLIC_SCRIPTS_OBJS =	\
	lprnlic

LPRNLIC_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(LPRNLIC_BASE_OBJS))) \
	$(addsuffix .o, $(LPRNLIC_SCRIPTS_OBJS))

lprnlic:	$(LPRNLIC_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

multiport:	multiport.o
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## mwl
MWL_BASE_OBJS =		\
	base64		\
	ds1990a		\
	licsig		\
	md5		\
	tki

MWL_SCRIPTS_OBJS =	\
	mwl

MWL_USB_OBJS =		\
	core		\
	key

MWL_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(MWL_BASE_OBJS))) \
	$(addsuffix .o, $(MWL_SCRIPTS_OBJS)) \
	$(addprefix $(DIR_USB), $(addsuffix .o, $(MWL_USB_OBJS)))

mwl:		$(MWL_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## rmbnklic
RMBNKLIC_BASE_OBJS =	\
	licsig		\
	md5		\
	rndtbl

RMBNKLIC_SCRIPTS_OBJS =	\
	rmbnklic	\
	rmlic

RMBNKLIC_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(RMBNKLIC_BASE_OBJS))) \
	$(addsuffix .o, $(RMBNKLIC_SCRIPTS_OBJS))

rmbnklic:	$(RMBNKLIC_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## rmbnklicx
RMBNKLICX_BASE_OBJS =	\
	base64		\
	licsig		\
	md5		\
	rndtbl		\
	tki

RMBNKLICX_SCRIPTS_OBJS =\
	rmbnklicx	\
	rmlic

RMBNKLICX_OBJS =	\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(RMBNKLICX_BASE_OBJS))) \
	$(addsuffix .o, $(RMBNKLICX_SCRIPTS_OBJS))

rmbnklicx:	$(RMBNKLICX_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## rmlprnlic
RMLPRNLIC_BASE_OBJS =	\
	licsig		\
	md5		\
	rndtbl

RMLPRNLIC_SCRIPTS_OBJS =\
	rmlprnlic	\
	rmlic

RMLPRNLIC_OBJS =	\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(RMLPRNLIC_BASE_OBJS))) \
	$(addsuffix .o, $(RMLPRNLIC_SCRIPTS_OBJS))

rmlprnlic:	$(RMLPRNLIC_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## rmlprnlicx
RMLPRNLICX_BASE_OBJS =	\
	base64		\
	licsig		\
	md5		\
	rndtbl		\
	tki

RMLPRNLICX_SCRIPTS_OBJS =	\
	rmlprnlicx	\
	rmlic

RMLPRNLICX_OBJS =	\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(RMLPRNLICX_BASE_OBJS))) \
	$(addsuffix .o, $(RMLPRNLICX_SCRIPTS_OBJS))

rmlprnlicx:	$(RMLPRNLICX_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## tpld
TPLD_BASE_OBJS =	\
	cfg		\
	hex		\
	serial

TPLD_SCRIPTS_OBJS =	\
	tpld

TPLD_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(TPLD_BASE_OBJS))) \
	$(addsuffix .o, $(TPLD_SCRIPTS_OBJS))

tpld:		$(TPLD_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## tstbeep
TSTBEEP_BASE_OBJS =	\
	serial

TSTBEEP_SCRIPTS_OBJS =	\
	tstbeep

TSTBEEP_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(TSTBEEP_BASE_OBJS))) \
	$(addsuffix .o, $(TSTBEEP_SCRIPTS_OBJS))

tstbeep:	$(TSTBEEP_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## tstcom
TSTCOM_BASE_OBJS =	\
	serial

TSTCOM_SCRIPTS_OBJS =	\
	tstcom

TSTCOM_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(TSTCOM_BASE_OBJS))) \
	$(addsuffix .o, $(TSTCOM_SCRIPTS_OBJS))

tstcom:		$(TSTCOM_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## unlkbnk
UNLKBNK_BASE_OBJS =	\
	licsig		\
	md5

UNLKBNK_SCRIPTS_OBJS =	\
	unlkbnk		\
	unlock

UNLKBNK_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(UNLKBNK_BASE_OBJS))) \
	$(addsuffix .o, $(UNLKBNK_SCRIPTS_OBJS))

unlkbnk:	$(UNLKBNK_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## unlklprn
UNLKLPRN_BASE_OBJS =	\
	licsig		\
	md5

UNLKLPRN_SCRIPTS_OBJS =	\
	unlklprn	\
	unlock

UNLKLPRN_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(UNLKLPRN_BASE_OBJS))) \
	$(addsuffix .o, $(UNLKLPRN_SCRIPTS_OBJS))

unlklprn:	$(UNLKLPRN_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## usbcheck
USBCHECK_USB_OBJS =	\
	core

USBCHECK_SCRIPTS_OBJS =	\
	usbcheck

USBCHECK_OBJS =		\
	$(addprefix $(DIR_USB), $(addsuffix .o, $(USBCHECK_USB_OBJS))) \
	$(addsuffix .o, $(USBCHECK_SCRIPTS_OBJS))

usbcheck:	$(USBCHECK_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## usbread
USBREAD_USB_OBJS =	\
	core

USBREAD_SCRIPTS_OBJS =	\
	usbread

USBREAD_OBJS =		\
	$(addprefix $(DIR_USB), $(addsuffix .o, $(USBREAD_USB_OBJS))) \
	$(addsuffix .o, $(USBREAD_SCRIPTS_OBJS))

usbread:	$(USBREAD_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
