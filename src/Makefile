SRCDIR=.
SRCS=$(wildcard $(SRCDIR)/*.cc)
OBJECTS=$(patsubst %.cc, %.o, $(SRCS))
CXXFLAGS=$(OPTLVL) -std=c++0x -I $(BOOSTHEADERS) -I $(YAMLHEADERS)

all: $(OBJECTS)

%.o: %.cc
	$(COMPILE.cc) -MD $< $(OUTPUT_OPTION);
	@cp $*.d $*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	    -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d;

-include $(SRCS:.cc=.P)

.PHONY: clean

clean:
	rm -f $(OBJECTS) *.P
