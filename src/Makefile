ifndef i686
ARCH         := $(shell uname -m)
ARCHITECTURE := $(shell echo $$(uname -m)-$$(uname -s | tr 'A-Z' 'a-z')-$$(uname -r | cut -d. -f1,2))
else
ARCH         := i686
ARCHITECTURE := i686-$$(uname -s | tr 'A-Z' 'a-z')-$$(uname -r | cut -d. -f1,2))
endif

$(warning "ARCH        :" $(ARCH))
$(warning "ARCHITECTURE:" $(ARCHITECTURE))

#ERL_INTERFACE = /home/qtsw/external/erlang/R12B-4.$(ARCH)/lib/erlang/lib/erl_interface-3.5.8
ERL_INTERFACE = /sw/external/erlang-R12B-4/$(ARCHITECTURE)/lib/erlang/lib/erl_interface-3.5.8
BOOST         = /sw/external/boost-1.35.0
#BOOST         = /sw/external/boost-1.37.0/$(ARCH)
LIBCWD        = /home/qtsw/lib/libcwd
LIBDIR        = ../lib.$(ARCHITECTURE)
OBJDIR        = ../obj.$(ARCHITECTURE)

LDFLAGS = -L$(ERL_INTERFACE)/lib -L$(BOOST)/lib -lei -lpthread -lboost_thread-gcc32-mt

CPPFLAGS = -DUSE_BOOST -Wall -g -fPIC -pthread -I$(ERL_INTERFACE)/include -I$(BOOST)/include

SOURCES=ComposedGuard.cpp EIBuffer.cpp EIConnection.cpp EIInputBuffer.cpp \
        EIOutputBuffer.cpp EITransport.cpp EpiAutoNode.cpp EpiBuffer.cpp \
        EpiConnection.cpp EpiException.cpp EpiLocalNode.cpp EpiMailBox.cpp \
        EpiMessage.cpp EpiNode.cpp EpiObserver.cpp EpiReceiver.cpp EpiSender.cpp \
        EpiUtil.cpp ErlAtom.cpp ErlBinary.cpp ErlConsList.cpp ErlDouble.cpp \
        ErlEmptyList.cpp ErlList.cpp ErlLong.cpp ErlPid.cpp ErlPort.cpp \
        ErlRef.cpp ErlString.cpp ErlTerm.cpp ErlTermFormat.cpp ErlTuple.cpp \
        ErlVariable.cpp ErlangTransportManager.cpp \
        GenericQueue.cpp MatchingCommandGuard.cpp PatternMatchingGuard.cpp \
        PlainBuffer.cpp Socket.cpp VariableBinding.cpp

ifdef DEBUG
SOURCES  += Debug.cpp
CPPFLAGS += -I$(LIBCWD)/include -DCWDEBUG -DLIBCWD_THREAD_SAFE
LDFLAGS  += -L$(LIBCWD)/lib -lcwd_r
endif

ALL_OBJECTS=$(SOURCES:%.cpp=$(OBJDIR)/%.o) 
SOME_OBJECTS=$(OBJDIR)/ErlTermFormat.o $(OBJDIR)/ErlTypesTest.o
REST_OBJECTS=$(filter-out $(SOME_OBJECTS), $(ALL_OBJECTS))
SAMPLE_OBJECTS=$(OBJDIR)/AutoNodeTest.o $(OBJDIR)/Sample.o $(OBJDIR)/Sample1.o

TARGET_DIRS=$(LIBDIR) $(OBJDIR)
TARGETS=$(LIBDIR)/libepi.a $(LIBDIR)/libepi.so ../erlTypesTest ../nodeSample ../nodeSample1 #../autoNodeTest 

all: $(TARGET_DIRS) $(TARGETS)

$(LIBDIR) $(OBJDIR):
	mkdir $@

$(LIBDIR)/libepi.so: $(ALL_OBJECTS)
	g++ -shared $(LDFLAGS) -o $@ $^

$(LIBDIR)/libepi.a: $(ALL_OBJECTS)
	ar -cru $@ $^

$(SOME_OBJECTS) : $(OBJDIR)/%.o : %.cpp
	g++ $(CPPFLAGS) -c -o $@ $<

$(REST_OBJECTS) : $(OBJDIR)/%.o : %.cpp %.hpp
	g++ $(CPPFLAGS) -c -o $@ $<

../erlTypesTest: $(OBJDIR)/ErlTypesTest.o $(LIBDIR)/libepi.so
	g++ $(CPPFLAGS) -o $@ $< $(LIBDIR)/libepi.a $(LDFLAGS)

../autoNodeTest: $(OBJDIR)/AutoNodeTest.o
	g++ $(CPPFLAGS) -o $@ $< $(LIBDIR)/libepi.a $(LDFLAGS)

$(OBJDIR)/AutoNodeTest.o: ../test/src/AutoNodeTest.cpp
	g++ $(CPPFLAGS) -o $@ $< $(LIBDIR)/libepi.a $(LDFLAGS) -I. -I../../src

../nodeSample: $(OBJDIR)/Sample.o
	g++ $(CPPFLAGS) -o $@ $< $(LIBDIR)/libepi.a $(LDFLAGS)

$(OBJDIR)/Sample.o: ../sample/Sample.cpp
	g++ -I $(PWD) $(CPPFLAGS) -c -o $@ $<

../nodeSample1: $(OBJDIR)/Sample1.o
	g++ $(CPPFLAGS) -o $@ $< $(LIBDIR)/libepi.a $(LDFLAGS)

$(OBJDIR)/Sample1.o: ../sample/Sample1.cpp
	g++ -I $(PWD) $(CPPFLAGS) -c -o $@ $<

test1:
	LD_LIBRARY_PATH=$(LIBCWD)/lib:$(BOOST)/lib:$$LD_LIBRARY_PATH ddd ../nodeSample1 &

info:
	@echo ALL_OBJECTS: $(ALL_OBJECTS)

clean:
	rm -f $(ALL_OBJECTS) $(LIBDIR)/libepi.a $(LIBDIR)/libepi.so
	rm -fr $(TARGET_DIRS)
