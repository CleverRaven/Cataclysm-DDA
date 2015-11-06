# Make any tests, and possibly run them.
# A selection of variables are exported from the master Makefile.

# As each test will have a main function we need to handle this file by file.
# We're using the fairly typical convention that any file ending in _test.cpp
# is a test executable.
SOURCES = $(wildcard *.cpp)
OBJS = $(SOURCES:%.cpp=$(ODIR)/%.o)

CATA_LIB=../cataclysm.a

ODIR := obj

DDIR := .deps

LDFLAGS += -L.

# Allow use of any header files from cataclysm.
# Catch sections throw unused variable warnings.
CXXFLAGS += -I../src -Wno-unused-variable

tests: cata_test

cata_test: $(ODIR) $(DDIR) $(OBJS)
	$(CXX) $(W32FLAGS) -o $@ $(DEFINES) $(OBJS) $(CATA_LIB) $(CXXFLAGS) $(LDFLAGS)

# Iterate over all the individual tests.
check: cata_test
	cd .. && tests/cata_test -d yes

clean:
	rm -f *.d $(ODIR)/*.o $(ODIR)/*.d

$(ODIR):
	mkdir $(ODIR)

$(DDIR):
	@mkdir $(DDIR)

$(ODIR)/%.o: %.cpp
	$(CXX) $(DEFINES) $(CXXFLAGS) -c $< -o $@

.PHONY: clean check tests

.SECONDARY: $(OBJS)
