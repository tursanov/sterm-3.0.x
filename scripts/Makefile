include Makefile.conf
DIR = scripts/

INCLUDE = ../include

BINARIES =		\
	dsd		\
	iplirhlp	\
	license		\
	mwl		\
	multiport	\
	rmlic		\
	rmlicx		\
	tpld		\
	tstbeep		\
	tstcom		\
	txl		\
	unlock		\
	usbcheck	\
	usbread		\
	xlicense

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
	express

SCRIPTS_OBJS =		\
	dsd		\
	iplirhlp	\
	license		\
	multiport	\
	mwl		\
	rmlic		\
	rmlicx		\
	tpld		\
	tstbeep		\
	tstcom		\
	txl		\
	unlock		\
	usbcheck	\
	usbread		\
	xlicense

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

## license
LICENSE_BASE_OBJS =	\
	md5

LICENSE_SCRIPTS_OBJS =	\
	license

LICENSE_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(LICENSE_BASE_OBJS))) \
	$(addsuffix .o, $(LICENSE_SCRIPTS_OBJS))

license:	$(LICENSE_OBJS)
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

## rmlic
RMLIC_BASE_OBJS =	\
	licsig		\
	md5		\
	rndtbl

RMLIC_SCRIPTS_OBJS =	\
	rmlic

RMLIC_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(RMLIC_BASE_OBJS))) \
	$(addsuffix .o, $(RMLIC_SCRIPTS_OBJS))

rmlic:		$(RMLIC_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## rmlicx
RMLICX_BASE_OBJS =	\
	base64		\
	licsig		\
	md5		\
	rndtbl		\
	tki

RMLICX_SCRIPTS_OBJS =	\
	rmlicx

RMLICX_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(RMLICX_BASE_OBJS))) \
	$(addsuffix .o, $(RMLICX_SCRIPTS_OBJS))

rmlicx:		$(RMLICX_OBJS)
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

## txl
TXL_BASE_OBJS =		\
	genfunc

TXL_LOG_OBJS =		\
	generic		\
	express

TXL_SCRIPTS_OBJS =	\
	txl

TXL_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(TXL_BASE_OBJS))) \
	$(addprefix $(DIR_LOG), $(addsuffix .o, $(TXL_LOG_OBJS))) \
	$(addsuffix .o, $(TXL_SCRIPTS_OBJS))

txl:		$(TXL_OBJS)
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

## unlock
UNLOCK_BASE_OBJS =	\
	md5

UNLOCK_SCRIPTS_OBJS =	\
	unlock

UNLOCK_OBJS =		\
	$(addprefix $(DIR_BASE), $(addsuffix .o, $(UNLOCK_BASE_OBJS))) \
	$(addsuffix .o, $(UNLOCK_SCRIPTS_OBJS))

unlock:		$(UNLOCK_OBJS)
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

xlicense:	xlicense.o ../base/md5.o
		@echo -e "\t$(LD_NAME)   $@"
		@$(CC) $(CFLAGS) -o $@ $^ $(LINKAGE)

clean:
		@rm -f $(BIN_GARBAGE)

ifneq ($(wildcard .depend),)
include .depend
endif
