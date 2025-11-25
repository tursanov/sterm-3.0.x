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

BEGIN_MSG	= "Building modules using Linux kernel tools..."
END_MSG		= "Modules build complete."
CLN_BEGIN_MSG	= "Cleaning modules using Linux kernel tools..."
CLN_END_MSG	= "Modules clean complete."

all:		begin_msg $(SUBDIRS) modules_install end_msg

begin_msg:
		@if [ -t 1 ]; then \
			echo -e "\033[01;32m$(BEGIN_MSG)\033[00m"; \
		else \
			echo -e $(BEGIN_MSG); \
		fi

end_msg:
		@if [ -t 1 ]; then \
			echo -e "\033[01;32m$(END_MSG)\033[00m"; \
		else \
			echo -e $(END_MSG); \
		fi

$(SUBDIRS):	
		@$(MAKE) -C $(KDIR) M=$(MODULES_DIR)/$@ PROJ_DIR=$(PWD) modules

clean:
		@if [ -t 1 ]; then \
			echo "\033[01;32m$(CLN_BEGIN_MSG)\033[00m"; \
		else \
			echo $(CLN_BEGIN_MSG); \
		fi
		@$(foreach DIR, $(SUBDIRS), $(MAKE) -C $(KDIR) M=$(MODULES_DIR)/$(DIR) clean;)
		@if [ -t 1 ]; then \
			echo "\033[01;32m$(CLN_END_MSG)\033[00m"; \
		else \
			echo $(CLN_END_MSG); \
		fi
		@rm -rf bin

modules_install:
		@rm -rf bin
		@mkdir bin
		@$(foreach MOD, $(SUBDIRS), cp $(MODULES_DIR)/$(MOD)/$(MOD).ko bin;)
