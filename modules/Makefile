include Makefile.conf

MODULES_ROOT = $(PWD)/modules

MODULES_DIR = $(MODULES_ROOT)/x86
SUBDIRS =		\
	chipset		\
	dallas		\
	oldxprn		\
	usbkey		\
	xprn

.PHONY:	$(SUBDIRS)

all:		begin_msg $(SUBDIRS) modules_install end_msg

begin_msg:
		@echo -e "\033[01;32mBuilding modules using Linux kernel tools...\033[00m"

end_msg:
		@echo -e "\033[01;32mModules build complete.\033[00m"

$(SUBDIRS):	
		@$(MAKE) -C $(KDIR) M=$(MODULES_DIR)/$@ PROJ_DIR=$(PWD) modules

clean:
		@echo -e "\033[01;32mCleaning modules using Linux kernel tools...\033[00m"
		@$(foreach DIR, $(SUBDIRS), $(MAKE) -C $(KDIR) M=$(MODULES_DIR)/$(DIR) clean;)
		@echo -e "\033[01;32mModules clean complete.\033[00m"
		@rm -rf bin

modules_install:
		@rm -rf bin
		@mkdir bin
		@$(foreach MOD, $(SUBDIRS), cp $(MODULES_DIR)/$(MOD)/$(MOD).ko bin;)
