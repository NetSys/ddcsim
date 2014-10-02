# TODO send object and prereq files to a user-set build directory
# A: easy if we use non-recursive make; hard as fuck otherwise

# TODO set these via ./configure?
# A: gotta spend a week reading the configure manual...

# TODO factor out common parts of makefile functionality to include files
# A: I've had enough Make'ing for some time.

# USER SETTINGS
OPTLVL=$(OPT)
EXEFILE=pilosim
GLOGLIB=/usr/local/lib/libglog.a
PROGOPTSLIB=/usr/local/lib/libboost_program_options.a
# END

CXX=g++
LDLIBS=-lpthread
STATICLIBS= $(GLOGLIB) $(PROGOPTSLIB)

SUBDIRS=src yaml-cpp-0.5.1
OPT=-O3
DBG=-O0 -g

export OPTLVL
export CXX

all: $(EXEFILE)

# TODO can this be removed by relying on the implicit rule %:%.o?
# TODO what is the best practice of finding the .o files to link?
$(EXEFILE): $(SUBDIRS)
	$(CXX) src/*.o yaml-cpp-0.5.1/src/*.o $(STATICLIBS) $(LDLIBS) $(OUTPUT_OPTION)

.PHONY: $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

# TODO is this actually necessary? Only if yaml-cpp's headers change
src: yaml-cpp-0.5.1

.PHONY: clean

clean:
	rm -f $(EXEFILE); \
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
