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

LDFLAGS += -L.

# Allow use of any header files from cataclysm.
# Catch sections throw unused variable warnings.
# Add no-sign-compare to fix MXE issue when compiling
# Catch also uses "#pragma gcc diagnostic", which is not recognized on some supported compilers.
# Clang and mingw are warning about Catch macros around perfectly normal boolean operations.
CXXFLAGS += -I../src -Wno-unused-variable -Wno-sign-compare -Wno-unknown-pragmas -Wno-parentheses -MMD -MP
CXXFLAGS += -Wall -Wextra

ifndef PCH
	PCH = 1
endif

ifndef CLANG
	CLANG = 0
endif

ifeq ($(PCH), 1)
	PCHFLAGS = -Ipch -Winvalid-pch -DCATA_CATCH_PCH
	PCH_H = pch/pch.hpp
	ifeq ($(CLANG), 0)
		PCH_P = pch/pch.hpp.gch
		PCHFLAGS += -fpch-preprocess -include pch.hpp
	else
		PCH_P = pch/pch.hpp.pch
		PCHFLAGS += -include-pch $(PCH_P)
		CXXFLAGS += -Wno-unused-macros -Xclang -fno-pch-timestamp
	endif
endif

ifeq ($(TARGETSYSTEM), WINDOWS)
  TEST_TARGET = $(BUILD_PREFIX)cata_test.exe
else
  TEST_TARGET = $(BUILD_PREFIX)cata_test
endif

tests: $(TEST_TARGET)

$(TEST_TARGET): $(OBJS) $(CATA_LIB)
	+$(CXX) $(W32FLAGS) -o $@ $(DEFINES) $(OBJS) $(CATA_LIB) $(CXXFLAGS) $(PCHFLAGS) $(LDFLAGS)

$(PCH_P): $(PCH_H)
	-$(CXX) $(CPPFLAGS) $(DEFINES) $(subst -Werror,,$(CXXFLAGS)) -Wno-non-virtual-dtor -Wno-unused-macros -I. -c $(PCH_H) -o $(PCH_P)

# Iterate over all the individual tests.
check: $(TEST_TARGET)
	cd .. && tests/$(TEST_TARGET) --min-duration 0.2 --rng-seed time

clean:
	rm -rf *obj *objwin
	rm -f *cata_test
	rm -f pch/pch.hpp.{gch,pch}

#Unconditionally create object directory on invocation.
$(shell mkdir -p $(ODIR))

$(ODIR)/%.o: %.cpp $(PCH_P)
	$(CXX) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) $(PCHFLAGS) -c $< -o $@

.PHONY: clean check tests precompile_header

.SECONDARY: $(OBJS)

-include ${OBJS:.o=.d}
