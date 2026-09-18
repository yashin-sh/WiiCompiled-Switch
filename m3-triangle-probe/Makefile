.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

ifeq ($(strip $(MESA_SWITCH_ROOT)),)
$(error "Set MESA_SWITCH_ROOT to the pinned mesa-switch checkout")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

TARGET      := WiiCompiled-Switch-m3-vulkan-triangle-probe
BUILD       := build
SOURCES     := source
INCLUDES    := $(MESA_SWITCH_ROOT)/include $(CURDIR)/generated
APP_TITLE   := WiiCompiled-Switch M3 Vulkan Triangle
APP_AUTHOR  := Community homebrew port
APP_VERSION := 0.1.0

MESA_VULKAN_LIB := $(MESA_SWITCH_ROOT)/builddir-switch/src/nouveau/vulkan/libvulkan.a
ifeq ($(wildcard $(MESA_VULKAN_LIB)),)
$(error "Missing $(MESA_VULKAN_LIB). Run scripts/build-m3-vulkan-triangle-probe.sh first")
endif

# Mesa's NAK/Rust objects are cross-built for aarch64-unknown-linux-gnu.
# The outer build helper resolves the matching Rust std closure from the
# container sysroot and passes absolute .rlib paths here.
RUST_STD_LIBS ?=
POSIX_COMPAT_LIBS ?=

ARCH        := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS      := -g -Wall -Wextra -O2 -ffunction-sections -fdata-sections $(ARCH)
CFLAGS      += $(INCLUDE) -D__SWITCH__ -DVK_USE_PLATFORM_VI_NN -DVK_NO_PROTOTYPES
CXXFLAGS    := $(CFLAGS) -std=gnu++20 -fno-rtti
ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) \
               -Wl,-Map,$(notdir $*.map) -Wl,--gc-sections -pthread \
               -Wl,-u,vk_icdGetInstanceProcAddr \
               -Wl,-u,vk_icdNegotiateLoaderICDInterfaceVersion
LIBS        := -Wl,--start-group $(MESA_VULKAN_LIB) $(RUST_STD_LIBS) \
               -lexpat -lzstd -lz -lnx -lstdc++ -lm $(POSIX_COMPAT_LIBS) \
               -Wl,--end-group
LIBDIRS     := $(PORTLIBS) $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT  := $(CURDIR)/$(TARGET)
export TOPDIR  := $(CURDIR)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))

export LD := $(CXX)
export OFILES := $(CPPFILES:.cpp=.o)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) \
                  -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)
export NROFLAGS += --nacp=$(CURDIR)/$(TARGET).nacp

.PHONY: all clean $(BUILD)
all: $(BUILD)

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@rm -fr build \
		$(TARGET).nro $(TARGET).nacp $(TARGET).elf $(TARGET).map

else

DEPENDS := $(OFILES:.o=.d)

.PHONY: all
all: $(OUTPUT).nro

$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif
