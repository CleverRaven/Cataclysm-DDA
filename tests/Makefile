# Make any tests, and possibly run them.
# A selection of variables are exported from the master Makefile.

# As each test will have a main function we need to handle this file by file.
# We're using the fairly typical convention that any file ending in _test.cpp
# is a test executable.
SOURCES = $(wildcard *_test.cpp)
OBJS = $(SOURCES:%.cpp=$(ODIR)/%.o)
TESTS = $(SOURCES:.cpp=)

CATA_LIB=../cataclysm.a

ODIR := obj

DDIR := .deps

LDFLAGS += -L. -lrt

# Allow use of any header files from cataclysm.
# Catch sections throw unused variable warnings.
CXXFLAGS += -I../src -Wno-unused-variable

LD := $(CROSS)g++

tests: $(TESTS)

%_test: $(ODIR) $(DDIR) $(OBJS)
	$(LD) $(W32FLAGS) -o $@ $(DEFINES) $(ODIR)/$@.o $(CATA_LIB) $(CXXFLAGS) $(LDFLAGS)

# Iterate over all the individual tests.
check: $(TESTS)
	$(foreach exe,$(TESTS),./$(exe);)

clean:
	rm -f $(TESTS) *.d $(ODIR)/*.o $(ODIR)/*.d

$(ODIR):
	mkdir $(ODIR)

$(DDIR):
	@mkdir $(DDIR)

$(ODIR)/%.o: %.cpp
	$(CXX) $(DEFINES) $(CXXFLAGS) -c $< -o $@

.PHONY: clean check tests
