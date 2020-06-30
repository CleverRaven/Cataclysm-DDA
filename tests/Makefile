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

ifeq ($(TARGETSYSTEM), WINDOWS)
  TEST_TARGET = $(BUILD_PREFIX)cata_test.exe
else
  TEST_TARGET = $(BUILD_PREFIX)cata_test
endif

tests: $(TEST_TARGET)

$(TEST_TARGET): $(OBJS) $(CATA_LIB)
	+$(CXX) $(W32FLAGS) -o $@ $(DEFINES) $(OBJS) $(CATA_LIB) $(CXXFLAGS) $(LDFLAGS)

# Iterate over all the individual tests.
check: $(TEST_TARGET)
	cd .. && tests/$(TEST_TARGET) -d yes --rng-seed time

clean:
	rm -rf *obj *objwin
	rm -f *cata_test

#Unconditionally create object directory on invocation.
$(shell mkdir -p $(ODIR))

$(ODIR)/%.o: %.cpp
	$(CXX) $(CPPFLAGS) $(DEFINES) $(CXXFLAGS) -c $< -o $@

.PHONY: clean check tests

.SECONDARY: $(OBJS)

-include ${OBJS:.o=.d}
