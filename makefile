LINUX_FLAGS    = -lm -lpthread
SOLARIS_FLAGS  = -lm -lrt -lpthread
SPECIFIC_FLAGS = $($(PLATFORM)_FLAGS)

CFLAGS = $(GLOBALFLAGS)

B = $(EW_HOME)/$(EW_VERSION)/bin
L = $(EW_HOME)/$(EW_VERSION)/lib

APP = nn_pick_ew

OBJS = \
	$(APP).o \
	compare.o \
	config.o \
	index.o \
	initvar.o \
	pick_ra.o \
	report.o \
	restart.o \
	sample.o \
	scan.o \
	sign.o \
	stalist.o

EW_LIBS = \
	$L/swap.o \
	$L/trheadconv.o \
	-L$L -lew_mt

$B/$(APP): $(OBJS)
	$(CC) -o $@ $(CFLAGS) $(OBJS) $(EW_LIBS) $(SPECIFIC_FLAGS)


# Clean-up rules
clean: PHONY
	-$(RM) a.out core *.o *.obj *% *~

clean_bin: PHONY
	-$(RM) $B/$(APP) $B/$(APP).exe

PHONY:
