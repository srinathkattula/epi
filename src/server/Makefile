ifndef i686
ARCH         := $(shell uname -m)
ARCHITECTURE := $(shell echo $$(uname -m)-$$(uname -s | tr '[A-Z]' '[a-z]')-$$(uname -r | cut -d. -f1,2))
else
ARCH         := i686
ARCHITECTURE := i686-$$(uname -s | tr '[A-Z]' '[a-z]')-$$(uname -r | cut -d. -f1,2))
endif

$(warning "ARCH        :" $(ARCH))
$(warning "ARCHITECTURE:" $(ARCHITECTURE))

#ERL_INTERFACE = /home/qtsw/external/erlang/R12B-4.$(ARCH)/lib/erlang/lib/erl_interface-3.5.8
ERL_INTERFACE = /sw/external/erlang-R12B-4/$(ARCHITECTURE)/lib/erlang/lib/erl_interface-3.5.8
BOOST         = /sw/external/boost-1.38.0/$(ARCH)
#BOOST         = /sw/external/boost-1.37.0/$(ARCH)
LIBDIR        = ../../lib.$(ARCHITECTURE)
OBJDIR        = ../../obj.$(ARCHITECTURE)

LDFLAGS = -L$(ERL_INTERFACE)/lib -L$(BOOST)/lib \
          -lei -lpthread $(BOOST)/lib/libboost_thread-gcc32-mt.a $(BOOST)/lib/libboost_system-gcc32-mt.a

CPPFLAGS = -Wall -DUSE_BOOST -Wall -g -fPIC -pthread -I$(ERL_INTERFACE)/include -I$(BOOST)/include -DSRV_DEBUG

SOURCES=main.cpp

ALL_OBJECTS=$(SOURCES:%.cpp=$(OBJDIR)/%.o) 

TARGET_DIRS=$(LIBDIR) $(OBJDIR)
TARGETS=../../ei_server_test

all: $(TARGET_DIRS) $(TARGETS)

$(LIBDIR) $(OBJDIR):
	mkdir $@

$(ALL_OBJECTS) : $(OBJDIR)/%.o : %.cpp $(wildcard *.hpp)
	g++ $(CPPFLAGS) -c -o $@ $<

../../ei_server_test: $(ALL_OBJECTS)
	g++ $(CPPFLAGS) -o $@ $^ $(LDFLAGS)

info:
	@echo ALL_OBJECTS: $(ALL_OBJECTS)

clean:
	rm -f $(ALL_OBJECTS) $(LIBDIR)/libepi.a $(LIBDIR)/libepi.so
	rm -fr $(TARGET_DIRS)

run:
	LD_LIBRARY_PATH=$(BOOST)/lib:$$LD_LIBRARY_PATH ../../ei_server_test 0.0.0.0 8000
