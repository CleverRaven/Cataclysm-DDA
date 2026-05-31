# Local production profile used by build-local-production-thinlto.sh.
#
# This file is loaded after the main Makefile so it can replace the default
# release optimization level with a local O3 + ThinLTO profile without editing
# upstream build rules.

LOCAL_CPU_FLAGS ?= -march=znver1 -mtune=znver1
LOCAL_OPT_FLAGS ?= -O3 $(LOCAL_CPU_FLAGS)
LOCAL_LTO_FLAGS ?= -flto=thin
LOCAL_LINKER_FLAGS ?= -fuse-ld=lld
LOCAL_THINLTO_CACHE ?= $(BUILD_DIR)/.thinlto-cache
LOCAL_THINLTO_LINK_FLAGS ?= -Wl,--thinlto-cache-dir=$(LOCAL_THINLTO_CACHE)

ifdef LOCAL_THINLTO_JOBS
  LOCAL_THINLTO_LINK_FLAGS += -Wl,--thinlto-jobs=$(LOCAL_THINLTO_JOBS)
endif

CXXFLAGS := $(filter-out -O% -flto%,$(CXXFLAGS))
LDFLAGS := $(filter-out -O% -flto%,$(LDFLAGS))

CXXFLAGS += $(LOCAL_OPT_FLAGS) $(LOCAL_LTO_FLAGS)
LDFLAGS += $(LOCAL_OPT_FLAGS) $(LOCAL_LTO_FLAGS) $(LOCAL_LINKER_FLAGS) $(LOCAL_THINLTO_LINK_FLAGS)

ifneq ($(CLANG),0)
  LLVM_AR := $(shell command -v llvm-ar 2>/dev/null)
  ifneq ($(LLVM_AR),)
    AR := $(LLVM_AR)
  endif
endif
