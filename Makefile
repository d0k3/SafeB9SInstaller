#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------

ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment. export DEVKITARM=<path to>devkitARM")
endif

include $(DEVKITARM)/ds_rules

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# DATA is a list of directories containing data files
# INCLUDES is a list of directories containing header files
# SPECS is the directory containing the important build and link files
#---------------------------------------------------------------------------------
export TARGET	:=	SafeB9SInstaller
BUILD		:=	build
SOURCES		:=	source source/common source/fs source/crypto source/fatfs source/nand source/safety
DATA		:=	data
INCLUDES	:=	source source/common source/font source/fs source/crypto source/fatfs source/nand source/safety

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
ARCH	:=	-mthumb -mthumb-interwork 

CFLAGS	:=	-g -Wall -Wextra -Wpedantic -Wcast-align -Wno-main -O2\
			-march=armv5te -mtune=arm946e-s -fomit-frame-pointer -ffast-math -std=gnu11\
			-fno-builtin-memcpy $(ARCH) -fdata-sections -ffunction-sections

CFLAGS	+=	$(INCLUDE) -DARM9

CFLAGS	+=	-DBUILD_NAME="\"$(TARGET) `date +'%Y/%m/%d'`\""

ifeq ($(FONT),ORIG)
CFLAGS	+=	-DFONT_ORIGINAL
else ifeq ($(FONT),6X10)
CFLAGS	+=	-DFONT_6X10
else ifeq ($(FONT),ACORN)
CFLAGS	+=	-DFONT_ACORN
else ifeq ($(FONT),GB)
CFLAGS	+=	-DFONT_GB
else
CFLAGS	+=	-DFONT_6X10
endif

ifeq ($(OPEN),1)
	CFLAGS += -DOPEN_INSTALLER
endif

CXXFLAGS	:= $(CFLAGS) -fno-rtti -fno-exceptions

ASFLAGS	:=	-g -mcpu=arm946e-s $(ARCH)
LDFLAGS	=	-T../link.ld -nostartfiles -g $(ARCH) -Wl,-Map,$(TARGET).map

LIBS	:=

#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS	:=

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------

export OUTPUT_D	:=	$(CURDIR)/output
export OUTPUT	:=	$(OUTPUT_D)/$(TARGET)
export RELEASE	:=	$(CURDIR)/release

export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
			$(foreach dir,$(DATA),$(CURDIR)/$(dir))

export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
#---------------------------------------------------------------------------------
	export LD	:=	$(CC)
#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
	export LD	:=	$(CXX)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

export OFILES	:= $(addsuffix .o,$(BINFILES)) \
			$(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			-I$(CURDIR)/$(BUILD)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: common clean all gateway firm 2xrsa binary cakehax cakerop brahma release

#---------------------------------------------------------------------------------
all: firm

common:
	@[ -d $(OUTPUT_D) ] || mkdir -p $(OUTPUT_D)
	@[ -d $(BUILD) ] || mkdir -p $(BUILD)

submodules:
	@-git submodule update --init --recursive

binary: common
	@make --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

firm: binary
	@firmtool build $(OUTPUT).firm -n 0x23F00000 -e 0 -D $(OUTPUT).bin -A 0x23F00000 -C NDMA -i

gateway: binary
	@cp resources/LauncherTemplate.dat $(OUTPUT_D)/Launcher.dat
	@dd if=$(OUTPUT).bin of=$(OUTPUT_D)/Launcher.dat bs=1497296 seek=1 conv=notrunc

2xrsa: binary
	@make --no-print-directory -C 2xrsa
	@mv $(OUTPUT).bin $(OUTPUT_D)/arm9.bin
	@mv $(CURDIR)/2xrsa/bin/arm11.bin $(OUTPUT_D)/arm11.bin

cakehax: submodules binary
	@make dir_out=$(OUTPUT_D) name=$(TARGET).dat -C CakeHax bigpayload
	@dd if=$(OUTPUT).bin of=$(OUTPUT).dat bs=512 seek=160

cakerop: cakehax
	@make DATNAME=$(TARGET).dat DISPNAME=$(TARGET) GRAPHICS=../resources/CakesROP -C CakesROP
	@mv CakesROP/CakesROP.nds $(OUTPUT_D)/$(TARGET).nds

brahma: submodules binary
	@[ -d BrahmaLoader/data ] || mkdir -p BrahmaLoader/data
	@cp $(OUTPUT).bin BrahmaLoader/data/payload.bin
	@cp resources/BrahmaAppInfo BrahmaLoader/resources/AppInfo
	@cp resources/BrahmaIcon.png BrahmaLoader/resources/icon.png
	@make --no-print-directory -C BrahmaLoader APP_TITLE=$(TARGET)
	@mv BrahmaLoader/output/*.3dsx $(OUTPUT_D)
	@mv BrahmaLoader/output/*.smdh $(OUTPUT_D)

release:
	@rm -fr $(BUILD) $(OUTPUT_D) $(RELEASE)
	@make --no-print-directory binary
	@-make --no-print-directory gateway
	@-make --no-print-directory firm
	@-make --no-print-directory 2xrsa
	@-make --no-print-directory cakerop
	@-make --no-print-directory brahma
	@[ -d $(RELEASE) ] || mkdir -p $(RELEASE)
	@[ -d $(RELEASE)/$(TARGET) ] || mkdir -p $(RELEASE)/$(TARGET)
	@cp $(OUTPUT).bin $(RELEASE)
	@-cp $(OUTPUT).firm $(RELEASE)
	@-cp $(OUTPUT_D)/arm9.bin $(RELEASE)
	@-cp $(OUTPUT_D)/arm11.bin $(RELEASE)
	@-cp $(OUTPUT).dat $(RELEASE)
	@-cp $(OUTPUT).nds $(RELEASE)
	@-cp $(OUTPUT).3dsx $(RELEASE)/$(TARGET)
	@-cp $(OUTPUT).smdh $(RELEASE)/$(TARGET)
	@-cp $(OUTPUT_D)/Launcher.dat $(RELEASE)
	@cp $(CURDIR)/README.md $(RELEASE)
	@-7z a $(RELEASE)/$(TARGET)-`date +'%Y%m%d-%H%M%S'`.zip $(RELEASE)/*

#---------------------------------------------------------------------------------
clean:
	@echo clean CakeHax...
	@-make clean --no-print-directory -C CakeHax
	@echo clean CakesROP...
	@-make clean --no-print-directory -C CakesROP
	@echo clean BrahmaLoader...
	@-make clean --no-print-directory -C BrahmaLoader
	@echo clean 2xrsa...
	@-make clean --no-print-directory -C 2xrsa
	@echo clean SafeB9SInstaller...
	@rm -fr $(BUILD) $(OUTPUT_D) $(RELEASE)


#---------------------------------------------------------------------------------
else

DEPENDS	:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
$(OUTPUT).bin	:	$(OUTPUT).elf
$(OUTPUT).elf	:	$(OFILES)


#---------------------------------------------------------------------------------
%.bin: %.elf
	@$(OBJCOPY) --set-section-flags .bss=alloc,load,contents -O binary $< $@
	@echo built ... $(notdir $@)


-include $(DEPENDS)


#---------------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------------
