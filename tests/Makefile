# Make any tests, and possibly run them.
# A selection of variables are exported from the master Makefile
# libtap.so and tap.h must be in the local directory or on the system path.

# ODIR is realative, so we can use the one from the main Makefile
TEST_SOURCES = $(wildcard *.cpp)
TEST__OBJS = $(TEST_SOURCES:.cpp=.o)
TEST_OBJS = $(patsubst %,$(ODIR)/%,$(TEST__OBJS))

# Brute force solution, relative paths to EVERY .o file
SOURCE_OBJS = $(patsubst %,../$(ODIR)/%,$(filter-out main.o,$(_OBJS)))

LDFLAGS += -L. -ltap -lpthread

CXXFLAGS += -I..

tests: line_test

line_test: $(ODIR) $(DDIR) $(TEST__OBJS)
	$(CXX) $(W32FLAGS) -o line_test $(DEFINES) $(TEST__OBJS) $(SOURCE_OBJS) $(LDFLAGS)

$(ODIR):
	mkdir $(ODIR)

$(DDIR):
	@mkdir $(DDIR)
