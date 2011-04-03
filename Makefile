.SUFFIXES:
ifeq ($(strip $(PSL1GHT)),)
$(error "PSL1GHT must be set in the environment.")
endif

include $(PSL1GHT)/Makefile.base

TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCE		:=	source
INCLUDE		:=	include
#DATA		:=	data
LIBS		:=	 $(PSL1GHT)/modules/spu_soundmodule.bin.a \
				-lspu_sound -lmod -laudio -lnet -lsysfs -lpngdec -lfont -lfreetype -lz -ltiny3d -lgcm_sys -lreality -lsysutil -lio -lsysmodule -lm

TC_ADD		:=	`date +%d%H%M`
ICON0		:=	ICON0.PNG
ifneq ($(strip $(BUILD_STEALTH)),)
TITLE		:=	IrisManager - v1.1 ($(TC_ADD))
APPID		:=	IMANAGER4
else
TITLE		:=	LEMMINGS™ Trial Version
APPID		:=	NPUA80034
endif
CONTENTID	:=	UP0001-$(APPID)_00-0000000000000000

CFLAGS		+= -g -O2 -Wall --std=gnu99 `$(PS3DEV)/host/ppu/bin/freetype-config --cflags`

CFLAGS		+= -D__MKDEF_MANAGER_DIR__="\"$(APPID)\"" -D__MKDEF_MANAGER_FULLDIR__="\"/dev_hdd0/game/$(APPID)\""

#PPU_EMBEDDED_SRCS += data355/patch.txt data355/payload.bin
CFLAGS		+=  -DUSE_MEMCPY_SYSCALL
CFLAGS		+=  -DWITH_CFW355
SOURCE		+=	source/payload355
INCLUDE		+=	include/payload355
DATA		:=	data355

WITH_GAMES_DIR	?=	GAMEZ

CFLAGS		+=	-D'__MKDEF_GAMES_DIR="$(WITH_GAMES_DIR)"'

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCE),$(CURDIR)/$(dir)) \
					$(foreach dir,$(DATA),$(CURDIR)/$(dir))
export BUILDDIR	:=	$(CURDIR)/$(BUILD)
export DEPSDIR	:=	$(BUILDDIR)

CFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.c)))
CXXFILES	:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES		:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:= $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.bin)))
VCGFILES	:= $(foreach dir,$(SOURCE),$(notdir $(wildcard $(dir)/*.vcg)))

export OFILES	:=	$(CFILES:.c=.o) \
					$(CXXFILES:.cpp=.o) \
					$(SFILES:.S=.o) \
					$(BINFILES:.bin=.bin.o) \
					$(VCGFILES:.vcg=.vcg.o)

export BINFILES	:=	$(BINFILES:.bin=.bin.h)
export VCGFILES	:=	$(VCGFILES:.vcg=.vcg.h)

export INCLUDES	:=	$(foreach dir,$(INCLUDE),-I$(CURDIR)/$(dir)) \
					-I$(CURDIR)/$(BUILD) -I$(PSL1GHT)/modules

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo Clean...
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).self $(OUTPUT).a $(OUTPUT).pkg

pkg: $(BUILD)
	@echo Creating PKG...
	@mkdir -p $(BUILD)/pkg
	@mkdir -p $(BUILD)/pkg/USRDIR
	@cp $(ICON0) $(BUILD)/pkg/
	@make_self_npdrm $(BUILD)/$(TARGET).elf $(BUILD)/pkg/USRDIR/EBOOT.BIN $(CONTENTID)
	@$(SFO) --title "$(TITLE)" --appid "$(APPID)" -f $(SFOXML) $(BUILD)/pkg/PARAM.SFO
	@$(PKG) --contentid $(CONTENTID) $(BUILD)/pkg/ $(OUTPUT).pkg

run: $(BUILD)
	@$(PS3LOADAPP) $(OUTPUT).self

else

DEPENDS	:= $(OFILES:.o=.d)

$(OUTPUT).self: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)
$(OFILES): $(BINFILES) $(VCGFILES)

-include $(DEPENDS)

endif
