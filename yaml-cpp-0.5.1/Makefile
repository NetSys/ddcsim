SRCDIR=src
SRCS=$(wildcard $(SRCDIR)/*.cpp)
OBJECTS=$(patsubst %.cpp, %.o, $(SRCS))
CXXFLAGS=$(OPTLVL) -Wno-deprecated -I $(BOOSTHEADERS) -I $(YAMLHEADERS)

all: $(OBJECTS)

%.o: %.cpp
	$(COMPILE.cpp) -MD $< $(OUTPUT_OPTION); \
	cp $*.d $*.P; \
	sed -e 's/#.*//' -e 's/^[^:]*: *//' -e 's/ *\\$$//' \
	    -e '/^$$/ d' -e 's/$$/ :/' < $*.d >> $*.P; \
	rm -f $*.d;

-include $(SRCS:.cpp=.P)

.PHONY: clean

clean:
	rm -f $(OBJECTS) *.P
