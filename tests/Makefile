# Make any tests, and possibly run them.
# A selection of variables are exported from the master Makefile.

# As each test will have a main function we need to handle this file by file.
# We're using the fairly typical convention that any file ending in _test.cpp
# is a test executable.
SOURCES = $(wildcard *.cpp)
OBJS = $(sort $(SOURCES:%.cpp=$(ODIR)/%.o))

CATA_LIB=../$(BUILD_PREFIX)cataclysm.a

# If you invoke this makefile directly and the parent directory was
# built with BUILD_PREFIX set, you must set it for this invocation as well.
ODIR ?= obj

LDFLAGS += -lpthread -L.

# Enabling benchmarks
DEFINES += -DCATCH_CONFIG_ENABLE_BENCHMARKING

# Allow use of any header files from cataclysm.
# Catch sections throw unused variable warnings.
# Add no-sign-compare to fix MXE issue when compiling
# Catch also uses "#pragma gcc diagnostic", which is not recognized on some supported compilers.
# Clang and mingw are warning about Catch macros around perfectly normal boolean operations.
CPPFLAGS += -I. -I../src -isystem ../src/third-party
CXXFLAGS += -Wno-unused-variable -Wno-sign-compare -Wno-unknown-pragmas -Wno-parentheses
CXXFLAGS += -Wall -Wextra

ifndef PCH
  PCH = 1
endif

ifndef CLANG
  CLANG = 0
endif

ifeq ($(PCH), 1)
PCHFLAGS += -DCATA_CATCH_PCH
PCH_H = pch/tests-pch.hpp
ifeq ($(CLANG), 0)
PCH_P = $(PCH_H).gch
else
PCH_P = $(PCH_H).pch
CXXFLAGS += -Wno-unused-macros
endif
endif

ifeq ($(TARGETSYSTEM), WINDOWS)
  TEST_TARGET = $(BUILD_PREFIX)cata_test.exe
else
  TEST_TARGET = $(BUILD_PREFIX)cata_test
endif

tests: $(TEST_TARGET)

$(TEST_TARGET): $(OBJS) $(CATA_LIB)
	+$(CXX) $(W32FLAGS) -o $@ $(DEFINES) $(OBJS) $(CATA_LIB) $(CPPFLAGS) $(CXXFLAGS) $(LDFLAGS)

$(PCH_P): $(PCH_H)
	-$(CXX) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) -MMD -MP -Wno-error -Wno-non-virtual-dtor -Wno-unused-macros -I. -c $(PCH_H) -o $(PCH_P)

TEST_BATCHES = "crafting_skill_gain" "[slow]\ ~crafting_skill_gain" "~[slow]\ ~[.]"

$(TEST_BATCHES): $(TEST_TARGET)
	cd .. && tests/$(TEST_TARGET) --min-duration 0.2 --rng-seed time $@

check: $(TEST_TARGET) $(TEST_BATCHES)

check-single: $(TEST_TARGET)
	cd .. && tests/$(TEST_TARGET) --min-duration 0.2 --rng-seed time

clean: clean-pch
	rm -rf *obj *objwin
	rm -f *cata_test

clean-pch:
	rm -f pch/*pch.hpp.gch
	rm -f pch/*pch.hpp.pch
	rm -f pch/*pch.hpp.d

#Unconditionally create object directory on invocation.
$(shell mkdir -p $(ODIR))

# Adding ../tests/ so that the directory appears in __FILE__ for log messages
$(ODIR)/%.o: %.cpp $(PCH_P)
	$(CXX) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) -MMD -MP $(subst main-pch,tests-pch,$(PCHFLAGS)) -c ../tests/$< -o $@

$(ODIR)/%.inc: %.cpp
	$(CXX) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) -H -E $< -o /dev/null 2> $@

.PHONY: includes
includes: $(OBJS:.o=.inc)

.PHONY: clean clean-pch check check-single tests precompile_header

.SECONDARY: $(OBJS)

-include ${OBJS:.o=.d}
