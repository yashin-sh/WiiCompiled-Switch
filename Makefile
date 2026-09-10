.SUFFIXES:

ifeq ($(strip $(DEVKITPRO)),)
$(error "Please set DEVKITPRO in your environment. export DEVKITPRO=<path to>/devkitpro")
endif

TOPDIR ?= $(CURDIR)
include $(DEVKITPRO)/libnx/switch_rules

MKW_SYNTHETIC_PRODUCT ?= 0
MKW_SYNTHETIC_DATA_INIT ?= 0
MKW_SYNTHETIC_FUNCTION_LINK ?= 0
MKW_SYNTHETIC_EXECUTION ?= 0
MKW_LOCAL_PRODUCT ?= 0
MKW_LOCAL_FUNCTION_SHARDS ?= 0
MKW_LOCAL_FUNCTION_EXECUTION ?= 0

TARGET      := WiiCompiled-Switch
BUILD       := build
UPSTREAM    := third_party/WiiCompiled
UPSTREAM_RUNTIME := $(UPSTREAM)/runtime
SOURCES     := source $(UPSTREAM_RUNTIME)/src/platform
INCLUDES    := include $(UPSTREAM_RUNTIME)/include
APP_VERSION := 0.0.6
TRANSLATED_LINK_MODE := 0
TRANSLATED_RETAIN_SYMBOL :=

ifeq ($(MKW_SYNTHETIC_PRODUCT),1)
ifneq ($(MKW_SYNTHETIC_DATA_INIT)$(MKW_SYNTHETIC_FUNCTION_LINK)$(MKW_SYNTHETIC_EXECUTION)$(MKW_LOCAL_PRODUCT)$(MKW_LOCAL_FUNCTION_SHARDS)$(MKW_LOCAL_FUNCTION_EXECUTION),000000)
$(error "Select only one product mode")
endif
TARGET      := WiiCompiled-Switch-synthetic-product
BUILD       := build-synthetic-product
SOURCES     += synthetic-product
DEFINES     += -DMKW_SYNTHETIC_PRODUCT=1
APP_VERSION := 0.0.6-synthetic
endif

ifeq ($(MKW_SYNTHETIC_DATA_INIT),1)
ifneq ($(MKW_SYNTHETIC_PRODUCT)$(MKW_SYNTHETIC_FUNCTION_LINK)$(MKW_SYNTHETIC_EXECUTION)$(MKW_LOCAL_PRODUCT)$(MKW_LOCAL_FUNCTION_SHARDS)$(MKW_LOCAL_FUNCTION_EXECUTION),000000)
$(error "Select only one product mode")
endif
TARGET      := WiiCompiled-Switch-synthetic-data-init
BUILD       := build-synthetic-data-init
SOURCES     += synthetic-data-init
DEFINES     += -DMKW_SYNTHETIC_DATA_INIT=1 -DMKW_ENABLE_DATA_INIT_HANDOFF=1
APP_VERSION := 0.0.6-data-init
endif

ifeq ($(MKW_SYNTHETIC_FUNCTION_LINK),1)
ifneq ($(MKW_SYNTHETIC_PRODUCT)$(MKW_SYNTHETIC_DATA_INIT)$(MKW_SYNTHETIC_EXECUTION)$(MKW_LOCAL_PRODUCT)$(MKW_LOCAL_FUNCTION_SHARDS)$(MKW_LOCAL_FUNCTION_EXECUTION),000000)
$(error "Select only one product mode")
endif
TARGET      := WiiCompiled-Switch-synthetic-function-link
BUILD       := build-synthetic-function-link
SOURCES     += synthetic-function-link
DEFINES     += -DMKW_SYNTHETIC_FUNCTION_LINK=1 -DMKW_TRANSLATED_LINK_ONLY=1
APP_VERSION := 0.0.6-synthetic-link
TRANSLATED_LINK_MODE := 1
TRANSLATED_RETAIN_SYMBOL := synthetic_translated_leaf
endif

ifeq ($(MKW_SYNTHETIC_EXECUTION),1)
ifneq ($(MKW_SYNTHETIC_PRODUCT)$(MKW_SYNTHETIC_DATA_INIT)$(MKW_SYNTHETIC_FUNCTION_LINK)$(MKW_LOCAL_PRODUCT)$(MKW_LOCAL_FUNCTION_SHARDS)$(MKW_LOCAL_FUNCTION_EXECUTION),000000)
$(error "Select only one product mode")
endif
TARGET      := WiiCompiled-Switch-synthetic-execution
BUILD       := build-synthetic-execution
SOURCES     += synthetic-data-init synthetic-execution
DEFINES     += -DMKW_SYNTHETIC_EXECUTION=1 -DMKW_ENABLE_DATA_INIT_HANDOFF=1 \
               -DMKW_ENABLE_TRANSLATED_EXECUTION_HANDOFF=1
APP_VERSION := 0.0.6-synthetic-exec
TRANSLATED_LINK_MODE := 1
TRANSLATED_RETAIN_SYMBOL := synthetic_translated_execution_leaf
endif

LOCAL_GENERATED_DIR := local-product/generated
LOCAL_SHARD_ROOT := $(LOCAL_GENERATED_DIR)/build_shards
ifeq ($(MKW_LOCAL_PRODUCT),1)
ifneq ($(MKW_SYNTHETIC_PRODUCT)$(MKW_SYNTHETIC_DATA_INIT)$(MKW_SYNTHETIC_FUNCTION_LINK)$(MKW_SYNTHETIC_EXECUTION)$(MKW_LOCAL_FUNCTION_SHARDS)$(MKW_LOCAL_FUNCTION_EXECUTION),000000)
$(error "Select only one product mode")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_GENERATED_DIR)/data_sections_init.cpp),)
$(error "Missing local-product/generated/data_sections_init.cpp. Run scripts/prepare-local-data-init.sh first")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_GENERATED_DIR)/data_sections_init_blobs.S),)
$(error "Missing local-product/generated/data_sections_init_blobs.S. Run scripts/prepare-local-data-init.sh first")
endif
TARGET      := WiiCompiled-Switch-local-product
BUILD       := build-local-product
SOURCES     += local-product-support $(LOCAL_GENERATED_DIR)
DEFINES     += -DMKW_LOCAL_PRODUCT=1 -DMKW_ENABLE_DATA_INIT_HANDOFF=1
APP_VERSION := 0.0.6-local
endif

ifeq ($(MKW_LOCAL_FUNCTION_SHARDS),1)
ifneq ($(MKW_SYNTHETIC_PRODUCT)$(MKW_SYNTHETIC_DATA_INIT)$(MKW_SYNTHETIC_FUNCTION_LINK)$(MKW_SYNTHETIC_EXECUTION)$(MKW_LOCAL_PRODUCT)$(MKW_LOCAL_FUNCTION_EXECUTION),000000)
$(error "Select only one product mode")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_GENERATED_DIR)/data_sections_init.cpp),)
$(error "Missing local-product/generated/data_sections_init.cpp. Run scripts/prepare-local-data-init.sh first")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_GENERATED_DIR)/data_sections_init_blobs.S),)
$(error "Missing local-product/generated/data_sections_init_blobs.S. Run scripts/prepare-local-data-init.sh first")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_GENERATED_DIR)/base_translation_output.json),)
$(error "Missing local translated metadata. Run scripts/prepare-local-function-shards.sh first")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_SHARD_ROOT)/shards.cmake),)
$(error "Missing local translated shard graph. Run scripts/prepare-local-function-shards.sh first")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_SHARD_ROOT)/base_common/*.cpp),)
$(error "Missing base_common translated function shards. Run scripts/prepare-local-function-shards.sh first")
endif
TARGET      := WiiCompiled-Switch-local-function-link
BUILD       := build-local-function-link
SOURCES     += local-product-support $(LOCAL_GENERATED_DIR) $(LOCAL_SHARD_ROOT)/base_common
ifneq ($(wildcard $(TOPDIR)/$(LOCAL_SHARD_ROOT)/base_portable_sensitive/*.cpp),)
SOURCES     += $(LOCAL_SHARD_ROOT)/base_portable_sensitive
endif
# Deliberately DO NOT add base_registration or base_dispatch here: their
# registrar objects execute static constructors before main. This checkpoint
# proves compile/link only and still stops after generated data initialization.
DEFINES     += -DMKW_LOCAL_PRODUCT=1 -DMKW_LOCAL_FUNCTION_SHARDS=1 \
               -DMKW_ENABLE_DATA_INIT_HANDOFF=1 -DMKW_TRANSLATED_LINK_ONLY=1
APP_VERSION := 0.0.6-local-link
TRANSLATED_LINK_MODE := 1
# The pinned PAL RMCP01 map names 0x8000609C as __get_debug_bba, while the
# translator emits the actual C++ symbol func_8000609C. Retain the emitted
# symbol so --gc-sections keeps a real translated function in the final ELF.
# The Switch runtime never calls it at this checkpoint.
TRANSLATED_RETAIN_SYMBOL := func_8000609C
endif

ifeq ($(MKW_LOCAL_FUNCTION_EXECUTION),1)
ifneq ($(MKW_SYNTHETIC_PRODUCT)$(MKW_SYNTHETIC_DATA_INIT)$(MKW_SYNTHETIC_FUNCTION_LINK)$(MKW_SYNTHETIC_EXECUTION)$(MKW_LOCAL_PRODUCT)$(MKW_LOCAL_FUNCTION_SHARDS),000000)
$(error "Select only one product mode")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_GENERATED_DIR)/data_sections_init.cpp),)
$(error "Missing local-product/generated/data_sections_init.cpp. Run scripts/prepare-local-data-init.sh first")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_GENERATED_DIR)/data_sections_init_blobs.S),)
$(error "Missing local-product/generated/data_sections_init_blobs.S. Run scripts/prepare-local-data-init.sh first")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_GENERATED_DIR)/RuntimeConfig.h),)
$(error "Missing local-product/generated/RuntimeConfig.h. Run scripts/prepare-local-data-init.sh first")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_GENERATED_DIR)/base_translation_output.json),)
$(error "Missing local translated metadata. Run scripts/prepare-local-function-shards.sh first")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_SHARD_ROOT)/shards.cmake),)
$(error "Missing local translated shard graph. Run scripts/prepare-local-function-shards.sh first")
endif
ifeq ($(wildcard $(TOPDIR)/$(LOCAL_SHARD_ROOT)/base_common/*.cpp),)
$(error "Missing base_common translated function shards. Run scripts/prepare-local-function-shards.sh first")
endif
TARGET      := WiiCompiled-Switch-local-function-exec
BUILD       := build-local-function-exec
SOURCES     += local-product-support local-execution-support $(LOCAL_GENERATED_DIR) $(LOCAL_SHARD_ROOT)/base_common
INCLUDES    += $(LOCAL_GENERATED_DIR)
ifneq ($(wildcard $(TOPDIR)/$(LOCAL_SHARD_ROOT)/base_portable_sensitive/*.cpp),)
SOURCES     += $(LOCAL_SHARD_ROOT)/base_portable_sensitive
endif
# Still exclude generated registration/dispatch shards. The bootstrap calls one
# known generated C++ function directly and stops immediately after its return.
DEFINES     += -DMKW_LOCAL_PRODUCT=1 -DMKW_LOCAL_FUNCTION_EXECUTION=1 \
               -DMKW_ENABLE_DATA_INIT_HANDOFF=1 \
               -DMKW_ENABLE_TRANSLATED_EXECUTION_HANDOFF=1
APP_VERSION := 0.0.6-local-exec
TRANSLATED_LINK_MODE := 1
TRANSLATED_RETAIN_SYMBOL := func_8000609C
endif

ifeq ($(wildcard $(TOPDIR)/$(UPSTREAM_RUNTIME)/include/host_context.h),)
$(error "Pinned WiiCompiled submodule is missing. Run: git submodule update --init --recursive")
endif

APP_TITLE   := WiiCompiled-Switch
APP_AUTHOR  := Community homebrew port

ARCH        := -march=armv8-a+crc+crypto -mtune=cortex-a57 -mtp=soft -fPIE
CFLAGS      := -g -Wall -Wextra -O2 -ffunction-sections $(ARCH) $(DEFINES)
CFLAGS      += $(INCLUDE) -D__SWITCH__ -DMKW_PLATFORM_SWITCH=1
# The real WiiCompiled runtime uses C++ exceptions for checked guest-memory
# faults and other host boundaries. Keep RTTI disabled, but do not compile the
# Horizon integration with -fno-exceptions now that upstream code is consumed.
CXXFLAGS    := $(CFLAGS) -std=gnu++20 -fno-rtti

ifeq ($(TRANSLATED_LINK_MODE),1)
# The pinned AArch64 translator runtime is Clang-oriented. devkitA64 uses GCC,
# so preinclude the one required attribute compatibility shim and preserve the
# PPC floating-point contraction policy used by upstream translated targets.
CXXFLAGS    += -include $(TOPDIR)/include/devkita64_gcc_compat.hpp \
               -fno-fast-math -ffp-contract=off -fno-tree-slp-vectorize
endif

ASFLAGS     := -g $(ARCH)
LDFLAGS     := -specs=$(DEVKITPRO)/libnx/switch.specs -g $(ARCH) -Wl,-Map,$(notdir $*.map)
ifeq ($(TRANSLATED_LINK_MODE),1)
# All function shards are compiled, but link-time GC keeps only reachable code.
# Force the selected translated proof function to remain auditable in the ELF.
LDFLAGS     += -Wl,--gc-sections -Wl,-u,$(TRANSLATED_RETAIN_SYMBOL)
endif
LIBS        := -lnx
LIBDIRS     := $(PORTLIBS) $(LIBNX)

ifneq ($(BUILD),$(notdir $(CURDIR)))

export OUTPUT  := $(CURDIR)/$(TARGET)
export TOPDIR  := $(CURDIR)
export VPATH   := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES       := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES     := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES       := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES_UPPER := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))

export LD := $(CXX)
export OFILES_SRC := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o) $(SFILES_UPPER:.S=.o)
export OFILES := $(OFILES_SRC)
export INCLUDE := $(foreach dir,$(INCLUDES),-I$(CURDIR)/$(dir)) \
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
	@rm -fr build build-synthetic-product build-synthetic-data-init \
		build-synthetic-function-link build-synthetic-execution \
		build-local-product build-local-function-link build-local-function-exec \
		WiiCompiled-Switch.nro WiiCompiled-Switch.nacp WiiCompiled-Switch.elf WiiCompiled-Switch.map \
		WiiCompiled-Switch-synthetic-product.nro WiiCompiled-Switch-synthetic-product.nacp \
		WiiCompiled-Switch-synthetic-product.elf WiiCompiled-Switch-synthetic-product.map \
		WiiCompiled-Switch-synthetic-data-init.nro WiiCompiled-Switch-synthetic-data-init.nacp \
		WiiCompiled-Switch-synthetic-data-init.elf WiiCompiled-Switch-synthetic-data-init.map \
		WiiCompiled-Switch-synthetic-function-link.nro WiiCompiled-Switch-synthetic-function-link.nacp \
		WiiCompiled-Switch-synthetic-function-link.elf WiiCompiled-Switch-synthetic-function-link.map \
		WiiCompiled-Switch-synthetic-execution.nro WiiCompiled-Switch-synthetic-execution.nacp \
		WiiCompiled-Switch-synthetic-execution.elf WiiCompiled-Switch-synthetic-execution.map \
		WiiCompiled-Switch-local-product.nro WiiCompiled-Switch-local-product.nacp \
		WiiCompiled-Switch-local-product.elf WiiCompiled-Switch-local-product.map \
		WiiCompiled-Switch-local-function-link.nro WiiCompiled-Switch-local-function-link.nacp \
		WiiCompiled-Switch-local-function-link.elf WiiCompiled-Switch-local-function-link.map \
		WiiCompiled-Switch-local-function-exec.nro WiiCompiled-Switch-local-function-exec.nacp \
		WiiCompiled-Switch-local-function-exec.elf WiiCompiled-Switch-local-function-exec.map

else

DEPENDS := $(OFILES:.o=.d)

.PHONY: all
all: $(OUTPUT).nro

$(OUTPUT).nro: $(OUTPUT).elf $(OUTPUT).nacp
$(OUTPUT).elf: $(OFILES)

-include $(DEPENDS)

endif
