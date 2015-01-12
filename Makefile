# TODO send object and prereq files to a user-set build directory
# A: easy if we use non-recursive make; hard as fuck otherwise

# TODO set these via ./configure?
# A: gotta spend a week reading the configure manual...

# TODO factor out common parts of makefile functionality to include files
# A: I've had enough Make'ing for some time.

# USER SETTINGS
# TODO force recompilation when these variables change
OPTLVL=$(DBG)
EXEFILE=pilosim
GLOGLIB=/usr/local/lib/libglog.a
PROGOPTSLIB=/home/sam/Documents/netsys/pilo/boost_1_56_0/stage/lib/libboost_program_options.a
SYSTEMLIB=/home/sam/Documents/netsys/pilo/boost_1_56_0/stage/lib/libboost_system.a
BOOSTHEADERS=/home/sam/Documents/netsys/pilo/boost_1_56_0
# END

# USER SETTINGS ON AWS
#PROGOPTSLIB=/usr/local/lib/libboost_program_options.a
#SYSTEMLIB=/usr/local/lib/libboost_system.a
#FLAGLIB=/usr/lib/x86_64-linux-gnu/libgflags.a
#BOOSTHEADERS=/usr/include/boost/
#STATICLIBS= $(GLOGLIB) $(PROGOPTSLIB) $(SYSTEMLIB) $(FLAGLIB)
# END

CXX=g++
LDLIBS=-lpthread
STATICLIBS= $(GLOGLIB) $(PROGOPTSLIB) $(SYSTEMLIB)
# TODO why doesn't this work with relative addressing using period?
YAMLHEADERS=$(CURDIR)/yaml-cpp-0.5.1/include/

SUBDIRS=src yaml-cpp-0.5.1
OPT=-O3 -D NDEBUG
DBG=-O0 -g
PRO=-O1 -pg -D NDEBUG
CACHE=-O3 -g -D NDEBUG
FASTDBG=-O2 -g -D NDEBUG

export OPTLVL
export CXX
export YAMLHEADERS
export BOOSTHEADERS

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
