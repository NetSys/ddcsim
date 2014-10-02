SRCDIR=.
SRCS=$(wildcard $(SRCDIR)/*.cc)
OBJECTS=$(patsubst %.cc, %.o, $(SRCS))

BOOST_ROOT=/usr/bin/boost_1_54_0/

CXXFLAGS=$(OPTLVL) -std=c++0x -I $(BOOST_ROOT)

all: $(OBJECTS)

%.o: %.cc
	$(COMPILE.cc) -MD $< $(OUTPUT_OPTION); \
	cp $*.d $*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	    -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d;

-include $(SRCS:.cc=.P)

.PHONY: clean

clean:
	rm -f $(OBJECTS) *.P
