#---------------------------------------------------------------------------------
.SUFFIXES:

ifeq ($(strip $(DEVKITPPC)),)
$(error "Please set DEVKITPPC in your environment. export DEVKITPPC=<path to>devkitPPC")
endif

include $(DEVKITPPC)/gamecube_rules

#---------------------------------------------------------------------------------
# Fix missing LD and ELF2DOL variable definitions
export LD	:=	$(PREFIX)gcc
export ELF2DOL	:=	elf2dol

#---------------------------------------------------------------------------------
TARGET		:=	hello_world
BUILD		:=	build
SOURCES		:=	source
INCLUDES	:=	.

#---------------------------------------------------------------------------------
CFLAGS	= -g -O2 -Wall $(MACHDEP) $(INCLUDE)
LDFLAGS	= -g $(MACHDEP) -Wl,-Map,$(notdir $@).map
LIBS	:= -logc -lm
LIBDIRS	:=

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT	:=	$(CURDIR)/$(TARGET)
export VPATH	:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)

CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))

export OFILES	:=	$(CFILES:.c=.o) $(SFILES:.s=.o)

export INCLUDE	:=	$(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
					$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
					-I$(CURDIR)/$(BUILD) \
					-I$(LIBOGC_INC)

export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
					-L$(LIBOGC_LIB)

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(OUTPUT).elf $(OUTPUT).dol

else

DEPENDS	:=	$(OFILES:.o=.d)

$(OUTPUT).dol: $(OUTPUT).elf
	@echo output ... $(notdir $@)
	@$(ELF2DOL) $< $@

$(OUTPUT).elf: $(OFILES)
	@echo linking ... $(notdir $@)
	@$(LD) $^ $(LDFLAGS) $(LIBPATHS) $(LIBS) -o $@

-include $(DEPENDS)

endif
