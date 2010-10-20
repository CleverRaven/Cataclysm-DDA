# comment these to toggle them as one sees fit.
#WARNINGS = -Wall
DEBUG = -g

ODIR = obj
DDIR = .deps

TARGET = cataclysm

OS  = $(shell uname -o)
CXX = g++

CFLAGS = $(WARNINGS) $(DEBUG) -MD

ifeq ($(OS), Msys)
LDFLAGS = -static -lpdcurses
else 
LDFLAGS = -lncurses
endif

SOURCES = $(wildcard *.cpp)
_OBJS = $(SOURCES:.cpp=.o)
OBJS = $(patsubst %,$(ODIR)/%,$(_OBJS))

all: $(TARGET)
	@

$(TARGET): $(ODIR) $(DDIR) $(OBJS)
	$(CXX) -o $(TARGET) $(CFLAGS) $(OBJS) $(LDFLAGS) 

$(ODIR):
	mkdir $(ODIR)

$(DDIR):
	@mkdir $(DDIR)

$(ODIR)/%.o: %.cpp
	$(CXX) $(CFLAGS) -c $< -o $@
	@cp $(ODIR)/$*.d $(DDIR)/$*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	-e '/^$$/ d' -e 's/$$/ :/' < $(ODIR)/$*.d >> $(DDIR)/$*.P; \
	rm -f $(ODIR)/$*.d

clean:
	rm -f $(TARGET) $(ODIR)/*.o

-include $(SOURCES:%.cpp=$(DEPDIR)/%.P)
