# EPOS Application Makefile

include ../makedefs

APP_PATH := $(subst _app,,$(APPLICATION))
ISDIR := $(shell test -d $(APPLICATION_PATH)$(APP_PATH) && echo 0)

ifeq ($(ISDIR), 0)
	SRC := $(shell find ./$(APPLICATION_PATH)$(APP_PATH)/src/ -mindepth 1 -name "*.cc" -type f)
	OBJS := $(subst .cc,.do,$(SRC))
else
	OBJS :=
endif


all: install

.SUFFIXES: .do .cc

.cc.do:	$(SRC)
		$(CXX) $(CXXFLAGS) -o $@ $<

$(APPLICATION_PATH)$(APPLICATION):	$(APPLICATION_PATH)$(APPLICATION).o $(LIB)/* $(OBJS)
		$(ALD) $(ALDFLAGS) -o $@ $(APPLICATION_PATH)$(APPLICATION).o $(OBJS)

$(APPLICATION_PATH)$(APPLICATION).o: $(APPLICATION_PATH)$(APPLICATION).cc $(SRC)
		$(ACC) $(ACCFLAGS) -o $@ $<

install: $(APPLICATION_PATH)$(APPLICATION)
		$(INSTALL) $(APPLICATION_PATH)$(APPLICATION) $(IMG)
		
clean:
		$(CLEAN) *.o $(APPLICATION_PATH)$(APPLICATION) $(OBJS)
