#  copyright (c) 2018 quackmore-ff@yahoo.com
#
.NOTPARALLEL:

# Local project and SDK reference
TOP_DIR:=$(abspath $(dir $(lastword $(MAKEFILE_LIST))))
CCFLAGS:= -I$(TOP_DIR)/src/include -I$(SDK_DIR)/include
CCPPFLAGS:= -I$(TOP_DIR)/src/include -I$(SDK_DIR)/include
#LDFLAGS:= -L$(TOP_DIR)/lib -L$(SDK_DIR)/lib -L$(SDK_DIR)/ld $(LDFLAGS)
LDFLAGS:= -L$(TOP_DIR)/lib -L$(SDK_DIR)/lib $(LDFLAGS)

boot = $(BOOT)
app = $(APP)
freqdiv = $(FREQDIV)
mode = $(MODE)
size_map = $(SPI_SIZE_MAP)
flash = $(FLASH_SIZE)
user_start_addr = 0x01000

LD_FILE = $(LDDIR)/eagle.app.v6.ld

ifneq ($(boot), none)
  ifneq ($(app),0)
		ifeq ($(LD_REF), 2048)
			LD_FILE = $(TOP_DIR)/ld/eagle.app.v6.$(boot).$(LD_REF).app$(app).ld
    else
			LD_FILE = $(LDDIR)/eagle.app.v6.$(boot).$(LD_REF).app$(app).ld
		endif
		BIN_NAME = user$(app).$(flash).$(boot).$(size_map)
  endif
else
    app = 0
endif

ifdef DEBUG
	CCFLAGS += -ggdb -O0
	CCPPFLAGS += -ggdb -O0
	LDFLAGS += -ggdb
else
	CCFLAGS += -Os
endif

#############################################################
# Define binary start address when app == 1 or app == 2 
#

ifeq ($(size_map), 2)
    ifeq ($(app), 2)
        user_start_addr=0x41000
    endif
endif
ifeq ($(size_map), 3)
    ifeq ($(app), 2)
        user_start_addr=0x81000
    endif
endif
ifeq ($(size_map), 4)
    ifeq ($(app), 2)
        user_start_addr=0x81000
    endif
endif
ifeq ($(size_map), 5)
    ifeq ($(app), 2)
        user_start_addr=0x101000
    endif
endif
ifeq ($(size_map), 6)
    ifeq ($(app), 2)
        user_start_addr=0x101000
    endif
endif
ifeq ($(size_map), 8)
    ifeq ($(app), 2)
        user_start_addr=0x101000
    endif
endif
ifeq ($(size_map), 9)
    ifeq ($(app), 2)
        user_start_addr=0x101000
    endif
endif

#############################################################
# Select compile
#
ifeq ($(OS),Windows_NT)
	# WIN32
	# We are under windows.
	ifeq ($(XTENSA_CORE),lx106)
		# It is xcc
		AR = xt-ar
		CC = xt-xcc
		CXX = xt-xcc
		NM = xt-nm
		CPP = xt-cpp
		OBJCOPY = xt-objcopy
    OBJDUMP = xt-objdump
		CCFLAGS += --rename-section .text=.irom0.text --rename-section .literal=.irom0.literal
	else 
		# It is gcc, may be cygwin
		# Can we use -fdata-sections?
		CCFLAGS += -ffunction-sections -fno-jump-tables -fdata-sections
		AR = xtensa-lx106-elf-ar
		CC = xtensa-lx106-elf-gcc
		CXX = xtensa-lx106-elf-g++
		NM = xtensa-lx106-elf-nm
		CPP = xtensa-lx106-elf-cpp
		OBJCOPY = xtensa-lx106-elf-objcopy
    OBJDUMP = xtensa-lx106-elf-objdump
	endif
	FIRMWAREDIR = ..\\bin\\
	ifndef COMPORT
		ESPPORT = com1
	else
		ESPPORT = $(COMPORT)
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),AMD64)
	# ->AMD64
	endif
	ifeq ($(PROCESSOR_ARCHITECTURE),x86)
	# ->IA32
	endif
else
	# We are under other system, may be Linux. Assume using gcc.
	# Can we use -fdata-sections?
	ifndef COMPORT
		ESPPORT = /dev/ttyUSB0
	else
		ESPPORT = $(COMPORT)
	endif
	CCFLAGS += -ffunction-sections -fno-jump-tables -fdata-sections
	AR = xtensa-lx106-elf-ar
	CC = xtensa-lx106-elf-gcc
	CXX = xtensa-lx106-elf-g++
	NM = xtensa-lx106-elf-nm
	CPP = xtensa-lx106-elf-gcc -E
	OBJCOPY = xtensa-lx106-elf-objcopy
	OBJDUMP = xtensa-lx106-elf-objdump
	FIRMWAREDIR = ../bin/
	UNAME_S := $(shell uname -s)
	
	ifeq ($(UNAME_S),Linux)
	# LINUX
	endif
	ifeq ($(UNAME_S),Darwin)
	# OSX
	endif
	
	UNAME_P := $(shell uname -p)
	ifeq ($(UNAME_P),x86_64)
	# ->AMD64
	endif
	ifneq ($(filter %86,$(UNAME_P)),)
	# ->IA32
	endif
	ifneq ($(filter arm%,$(UNAME_P)),)
	# ->ARM
	endif
endif
#############################################################
ESPTOOL = esptool.py


CSRCS ?= $(wildcard *.c)
CXXSRCS ?= $(wildcard *.cpp)
ASRCs ?= $(wildcard *.s)
ASRCS ?= $(wildcard *.S)
SUBDIRS ?= $(patsubst %/,%,$(dir $(filter-out tools/Makefile,$(wildcard */Makefile))))

ODIR := .output
OBJODIR := $(ODIR)/$(TARGET)/$(FLAVOR)/obj

OBJS := $(CSRCS:%.c=$(OBJODIR)/%.o) \
        $(CXXSRCS:%.cpp=$(OBJODIR)/%.o) \
        $(ASRCs:%.s=$(OBJODIR)/%.o) \
        $(ASRCS:%.S=$(OBJODIR)/%.o)

DEPS := $(CSRCS:%.c=$(OBJODIR)/%.d) \
        $(CXXSCRS:%.cpp=$(OBJODIR)/%.d) \
        $(ASRCs:%.s=$(OBJODIR)/%.d) \
        $(ASRCS:%.S=$(OBJODIR)/%.d)

LIBODIR := $(ODIR)/$(TARGET)/$(FLAVOR)/lib
OLIBS := $(GEN_LIBS:%=$(LIBODIR)/%)

IMAGEODIR := $(ODIR)/$(TARGET)/$(FLAVOR)/image
OIMAGES := $(GEN_IMAGES:%=$(IMAGEODIR)/%)

BINODIR := $(ODIR)/$(TARGET)/$(FLAVOR)/bin
OBINS := $(GEN_BINS:%=$(BINODIR)/%)

GIT_VERSION := $(shell git --no-pager describe --tags --always --dirty)

#
# Note: 
# https://gcc.gnu.org/onlinedocs/gcc/Optimize-Options.html
# If you add global optimize options like "-O2" here 
# they will override "-Os" defined above.
# "-Os" should be used to reduce code size
#
CCFLAGS += 			                         \
	-g			                               \
	-Wpointer-arith		                     \
	-Wundef			                           \
	-Werror			                           \
	-Wl,-EL			                           \
	-nostdlib                              \
	-mlongcalls	                           \
	-mtext-section-literals                \
	-ffunction-sections                    \
	-fdata-sections	                       \
	-fno-builtin-printf                    \
	-fno-exceptions                        \
  -Wno-write-strings                     \
  -DESPBOT=1                             \
  -DAPP_RELEASE=\"$(GIT_VERSION)\"    \
	-DSPI_FLASH_SIZE_MAP=$(SPI_SIZE_MAP)
#	-Wall			
#	-fno-inline-functions	                 \

CCPPFLAGS += 			                         \
	-g			                               \
	-Wpointer-arith		                     \
	-Wundef			                           \
	-Werror			                           \
	-Wl,-EL			                           \
	-nostdlib                              \
	-mlongcalls	                           \
	-mtext-section-literals                \
	-ffunction-sections                    \
	-fdata-sections	                       \
	-fno-builtin-printf                    \
	-fno-exceptions                        \
	-fno-rtti                              \
  -Wno-write-strings                     \
  -DESPBOT=1                             \
  -DAPP_RELEASE=\"$(GIT_VERSION)\"    \
	-DSPI_FLASH_SIZE_MAP=$(SPI_SIZE_MAP)
#	-Wall			
#	-fno-inline-functions	                 \



CFLAGS = $(CCFLAGS) $(DEFINES) $(EXTRA_CCFLAGS) $(STD_CFLAGS) $(INCLUDES)
CPPFLAGS = $(CCPPFLAGS) $(DEFINES) $(EXTRA_CCFLAGS) $(STD_CFLAGS) $(INCLUDES)
DFLAGS = $(CCFLAGS) $(DDEFINES) $(EXTRA_CCFLAGS) $(STD_CFLAGS) $(INCLUDES)


#############################################################
# Functions
#

define ShortcutRule
$(1): .subdirs $(2)/$(1)
endef

define MakeLibrary
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
$$(LIBODIR)/$(1).a: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	@mkdir -p $$(LIBODIR)
	$$(if $$(filter %.a,$$?),mkdir -p $$(EXTRACT_DIR)_$(1))
	$$(if $$(filter %.a,$$?),cd $$(EXTRACT_DIR)_$(1); $$(foreach lib,$$(filter %.a,$$?),$$(AR) xo $$(UP_EXTRACT_DIR)/$$(lib);))
	$$(AR) ru $$@ $$(filter %.o,$$?) $$(if $$(filter %.a,$$?),$$(EXTRACT_DIR)_$(1)/*.o)
	$$(if $$(filter %.a,$$?),$$(RM) -r $$(EXTRACT_DIR)_$(1))
	@cp $$(LIBODIR)/$(1).a $(TOP_DIR)/lib/$(1).a
endef

define MakeImage
DEP_LIBS_$(1) = $$(foreach lib,$$(filter %.a,$$(COMPONENTS_$(1))),$$(dir $$(lib))$$(LIBODIR)/$$(notdir $$(lib)))
DEP_OBJS_$(1) = $$(foreach obj,$$(filter %.o,$$(COMPONENTS_$(1))),$$(dir $$(obj))$$(OBJODIR)/$$(notdir $$(obj)))
$$(IMAGEODIR)/$(1).out: $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1)) $$(DEPENDS_$(1))
	@mkdir -p $$(IMAGEODIR)
	$$(CC) $$(LDFLAGS) $$(if $$(LINKFLAGS_$(1)),$$(LINKFLAGS_$(1)),$$(LINKFLAGS_DEFAULT) $$(OBJS) $$(DEP_OBJS_$(1)) $$(DEP_LIBS_$(1))) -o $$@ 
endef

$(BINODIR)/%.bin: $(IMAGEODIR)/%.out
	@mkdir -p $(BINODIR)
	
ifeq ($(APP), 0)
	@$(RM) -r ../bin/eagle.S ../bin/eagle.dump
	@$(OBJDUMP) -x -s $< > ../bin/eagle.dump
	@$(OBJDUMP) -S $< > ../bin/eagle.S
else
	mkdir -p ../bin/upgrade
	@$(RM) -r ../bin/upgrade/$(BIN_NAME).S ../bin/upgrade/$(BIN_NAME).dump
	@$(OBJDUMP) -x -s $< > ../bin/upgrade/$(BIN_NAME).dump
	@$(OBJDUMP) -S $< > ../bin/upgrade/$(BIN_NAME).S
endif

	@$(OBJCOPY) --only-section .text -O binary $< eagle.app.v6.text.bin
	@$(OBJCOPY) --only-section .data -O binary $< eagle.app.v6.data.bin
	@$(OBJCOPY) --only-section .rodata -O binary $< eagle.app.v6.rodata.bin
	@$(OBJCOPY) --only-section .irom0.text -O binary $< eagle.app.v6.irom0text.bin

	@echo ""
	@echo "!!!"
	@echo "VERSION: "$(GIT_VERSION)
	
ifeq ($(app), 0)
	@python $(SDK_DIR)/tools/gen_appbin.py $< 0 $(mode) $(freqdiv) $(size_map) $(app)
	@mv eagle.app.flash.bin ../bin/eagle.flash.bin
	@mv eagle.app.v6.irom0text.bin ../bin/eagle.irom0text.bin
	@rm eagle.app.v6.*
	@echo "No boot needed."
	@echo "Generate eagle.flash.bin and eagle.irom0text.bin successully in folder bin."
	@echo "eagle.flash.bin-------->0x00000"
	@echo "eagle.irom0text.bin---->0x10000"
else
  ifneq ($(boot), new)
		@python $(SDK_DIR)/tools/gen_appbin.py $< 1 $(mode) $(freqdiv) $(size_map) $(app)
		@echo "Support boot_v1.1 and +"
  else
		@python $(SDK_DIR)/tools/gen_appbin.py $< 2 $(mode) $(freqdiv) $(size_map) $(app)

    ifeq ($(size_map), 6)
		  @echo "Support boot_v1.4 and +"
    else
      ifeq ($(size_map), 5)
	    	@echo "Support boot_v1.4 and +"
      else
		    @echo "Support boot_v1.2 and +"
      endif
    endif
  endif

	@echo $(GIT_VERSION) > ../bin/upgrade/www/version.txt
	@cp eagle.app.flash.bin ../bin/upgrade/www/user$(app).bin
	@mv eagle.app.flash.bin ../bin/upgrade/$(BIN_NAME).bin
	@rm eagle.app.v6.*
	@echo "Generate $(BIN_NAME).bin successully in folder bin/upgrade."
	@echo "boot.bin------------>0x00000"
	@echo "(Pick up boot.bin in folder $(SDK_DIR)/bin)"
	@echo "$(BIN_NAME).bin--->$(user_start_addr)"
endif

	@echo "!!!"
#############################################################
# Rules base
# Should be done in top-level makefile only
#

all:	.subdirs $(OBJS) $(OLIBS) $(OIMAGES) $(OBINS) $(SPECIAL_MKTARGETS)
ifndef SDK_DIR
  	$(error SDK_DIR is undefined. Generate the environment variables using 'gen_env.sh' or load them with '. env.sh')
endif


clean:
	$(foreach d, $(SUBDIRS), $(MAKE) -C $(d) clean;)
	$(RM) -r $(ODIR)/$(TARGET)/$(FLAVOR)
	$(RM) -r "$(TOP_DIR)/sdk"

clobber: $(SPECIAL_CLOBBER)
	$(foreach d, $(SUBDIRS), $(MAKE) -C $(d) clobber;)
	$(RM) -r $(ODIR)

flash:
ifeq ($(app), 0)
	$(ESPTOOL) --port $(ESPPORT) $(FLASH_OPTIONS) 0x00000 bin/eagle.flash.bin 0x10000 bin/eagle.irom0text.bin
else
	$(ESPTOOL) --port $(ESPPORT) $(FLASH_OPTIONS) $(user_start_addr) bin/upgrade/$(BIN_NAME).bin
endif

flash_erase:
	$(ESPTOOL) --port $(ESPPORT) erase_flash

flash_boot:
	$(ESPTOOL) --port $(ESPPORT) $(FLASH_OPTIONS) 0x00000 $(SDK_DIR)/bin/boot_v1.7.bin

flash_init:
	$(ESPTOOL) --port $(ESPPORT) $(FLASH_OPTIONS) $(FLASH_INIT)

.subdirs:
	@set -e; $(foreach d, $(SUBDIRS), $(MAKE) -C $(d);)

#.subdirs:
#	$(foreach d, $(SUBDIRS), $(MAKE) -C $(d))

ifneq ($(MAKECMDGOALS),clean)
ifneq ($(MAKECMDGOALS),clobber)
ifdef DEPS
sinclude $(DEPS)
endif
endif
endif


$(OBJODIR)/%.o: %.c
	@mkdir -p $(OBJODIR);
	$(CC) $(if $(findstring $<,$(DSRCS)),$(DFLAGS),$(CFLAGS)) $(COPTS_$(*F)) -o $@ -c $<

$(OBJODIR)/%.d: %.c
	@mkdir -p $(OBJODIR);
	@echo DEPEND: $(CC) -M $(CFLAGS) $<
	@set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.cpp
	@mkdir -p $(OBJODIR);
	$(CXX) $(if $(findstring $<,$(DSRCS)),$(DFLAGS),$(CPPFLAGS)) $(COPTS_$(*F)) -o $@ -c $<

$(OBJODIR)/%.d: %.cpp
	@mkdir -p $(OBJODIR);
	@echo DEPEND: $(CXX) -M $(CFLAGS) $<
	@set -e; rm -f $@; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.s
	@mkdir -p $(OBJODIR);
	$(CC) $(CFLAGS) -o $@ -c $<

$(OBJODIR)/%.d: %.s
	@mkdir -p $(OBJODIR); \
	set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(OBJODIR)/%.o: %.S
	@mkdir -p $(OBJODIR);
	$(CC) $(CFLAGS) -D__ASSEMBLER__ -o $@ -c $<

$(OBJODIR)/%.d: %.S
	@mkdir -p $(OBJODIR); \
	set -e; rm -f $@; \
	$(CC) -M $(CFLAGS) $< > $@.$$$$; \
	sed 's,\($*\.o\)[ :]*,$(OBJODIR)/\1 $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(foreach lib,$(GEN_LIBS),$(eval $(call ShortcutRule,$(lib),$(LIBODIR))))

$(foreach image,$(GEN_IMAGES),$(eval $(call ShortcutRule,$(image),$(IMAGEODIR))))

$(foreach bin,$(GEN_BINS),$(eval $(call ShortcutRule,$(bin),$(BINODIR))))

$(foreach lib,$(GEN_LIBS),$(eval $(call MakeLibrary,$(basename $(lib)))))

$(foreach image,$(GEN_IMAGES),$(eval $(call MakeImage,$(basename $(image)))))

#############################################################
# Recursion Magic - Don't touch this!!
#
# Each subtree potentially has an include directory
#   corresponding to the common APIs applicable to modules
#   rooted at that subtree. Accordingly, the INCLUDE PATH
#   of a module can only contain the include directories up
#   its parent path, and not its siblings
#
# Required for each makefile to inherit from the parent
#

INCLUDES := $(INCLUDES) -I $(PDIR)include -I $(PDIR)include/$(TARGET)
PDIR := ../$(PDIR)
sinclude $(PDIR)Makefile
