ifeq ($(strip $(DEVKITARM)),)
$(error "DEVKITARM is not set. Local: export DEVKITPRO=$$HOME/devkitpro DEVKITARM=$$HOME/devkitpro/devkitARM + PATH com arm-none-eabi-gcc e ndstool. Oficial: instale devkitPro/devkitARM.")
endif

include $(DEVKITARM)/ds_rules

TARGET := ohmyvictoria_ds
BUILD := build
SOURCES := source source/gen
INCLUDES := include
# ARM9 real: armv5te + arm946e-s (== template oficial; sem isso o GCC assume
# armv4t e o codigo perde instrucoes DSP/BLX do ARM9). -O2 seguro p/ DS.
ARCH := -march=armv5te -mtune=arm946e-s -mthumb -mthumb-interwork
# v0.7: -O2 p/ ARM9, ndstool gera .nds pronto p/ flashcard.
# Uso: make  (limpa: make clean). Requer DEVKITARM+LIBNDS (+calico no r68+).
GAME_TITLE := OH MY VICTORIA DS!
GAME_SUBTITLE1 := v0.8 caneta+mapa
GAME_SUBTITLE2 := aba de obras/mercado   
CFLAGS := -g -Wall -Wextra -O2 -ffunction-sections -fdata-sections $(ARCH) $(INCLUDE) -DARM9
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS := -g $(ARCH)
LDFLAGS := -specs=ds_arm9.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS := -lnds9
LIBDIRS := $(LIBNDS)
# Toolchain local rootless (este ambiente): newlib genuine do devkitARM + specs
# locais. No devkitPro oficial este bloco fica desligado (driver acha tudo).
ifdef LOCAL_TOOLCHAIN
CFLAGS += -I$(DEVKITARM)/arm-none-eabi/include
# -u _exit: marca _exit como indefinido desde o inicio p/ o --start-group
# extrair _exit.o do sysbase (o `exit` so aparece nas libs padrao do driver,
# DEPOIS do grupo). _exit do sysbase trava em loop — nunca chamado no jogo.
# -u extra: newlib (stdio/malloc, puxada pelas libs padrao do driver DEPOIS
# do grupo) referencia _sbrk_r/locks; pre-marca p/ o grupo extrair do sysbase.
LDFLAGS += -L$(DEVKITARM)/arm-none-eabi/lib/thumb -Wl,-u,_exit -Wl,-u,_sbrk_r -Wl,-u,__libc_lock_acquire_recursive -Wl,-u,__libc_lock_release_recursive -Wl,-u,__libc_lock_acquire -Wl,-u,__libc_lock_release -Wl,-u,_read_r -Wl,-u,_write_r -Wl,-u,_lseek_r -Wl,-u,_close_r -Wl,-u,_fstat_r -Wl,-u,_isatty_r
# ld single-pass: calico (via _EXTRALIBS, depois) referencia sysbase;
# --start-group permite re-scan (igual ao specs padrao do devkitARM oficial).
LIBS += -L$(DEVKITARM)/local-lib -Wl,--start-group -lnds9 -lsyncnone -lsysbase -lnosys -lcalico_ds9 -Wl,--end-group
endif

ifneq ($(BUILD),$(notdir $(CURDIR)))
export OUTPUT := $(CURDIR)/$(TARGET)
export TOPDIR := $(CURDIR)
export VPATH := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
ifeq ($(strip $(CPPFILES)),)
export LD := $(CC)
else
export LD := $(CXX)
endif
export OFILES := $(CFILES:.c=.o)

export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
	$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
	-I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: all clean $(BUILD)
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).nds $(TARGET).elf $(TARGET).map $(TARGET).lst

else
DEPENDS := $(OFILES:.o=.d)
$(OUTPUT).nds: $(OUTPUT).elf
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif
