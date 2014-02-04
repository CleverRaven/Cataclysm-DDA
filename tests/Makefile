# Make any tests, and possibly run them.
# A selection of variables are exported from the master Makefile
# libtap.so and tap.h must be in the local directory or on the system path.

# ODIR is realative, so we can use the one from the main Makefile

# As each test will have a main function we need to handle this file by file
TEST_SOURCES = $(wildcard *_test.cpp)
TEST_OBJS = $(TEST_SOURCES:%.cpp=$(ODIR)/%.o)
TESTS = $(TEST_SOURCES:.cpp=)

# Brute force solution, relative paths to EVERY .o file
SOURCE_OBJS = $(patsubst %,../$(ODIR)/%,$(filter-out main.o,$(_OBJS)))

LDFLAGS += -L. -ltap -lpthread

CXXFLAGS += -I..

LD := $(CROSS)g++

tests: $(TESTS)

%_test: $(ODIR) $(DDIR) $(SOURCE_OBJS) $(TEST_OBJS)
	$(LD) $(W32FLAGS) -o $@ $(DEFINES) $(ODIR)/$@.o $(SOURCE_OBJS) $(CXXFLAGS) $(LDFLAGS)

check: $(TESTS)
	LD_LIBRARY_PATH=/usr/local/lib ./$?

clean:
	rm -f $(TESTS) *.d $(ODIR)/*.o $(ODIR)/*.d

$(ODIR):
	mkdir $(ODIR)

$(DDIR):
	@mkdir $(DDIR)

$(ODIR)/%.o: %.cpp
	$(CXX) $(DEFINES) $(CXXFLAGS) -c $< -o $@

.PHONY: clean
