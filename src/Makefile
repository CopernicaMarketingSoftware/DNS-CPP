CPP	        		= g++
RM	        		= rm -f
CPPFLAGS			= -Wall -c -I../include -std=c++17 -MD
LD	        		= g++
LD_FLAGS			= -Wall -shared
SHARED_LIB			= lib$(LIBRARY_NAME).so.$(VERSION)
STATIC_LIB			= lib$(LIBRARY_NAME).a.$(VERSION)
SOURCES				= $(wildcard *.cpp)
SHARED_OBJECTS		= $(SOURCES:%.cpp=%.o)
DEPENDENCIES		= $(SOURCES:%.cpp=%.d)

ifeq ($(shell uname -s),Darwin)
    SONAMEPARAMETER = -install_name
else
    SONAMEPARAMETER = -soname
endif

-include ${DEPENDENCIES}

all:			CPPFLAGS += -g
all:			LD_FLAGS += -g
all:			shared static

release:		CPPFLAGS += -O2
release:		LD_FLAGS += -O2
release:		shared static

shared:			${SHARED_OBJECTS} ${SHARED_LIB}
static:			${SHARED_OBJECTS} ${STATIC_LIB}

${SHARED_LIB}: ${SHARED_OBJECTS}
	${LD} ${LD_FLAGS} -Wl,${SONAMEPARAMETER},lib$(LIBRARY_NAME).so.$(SONAME) -o $@ ${SHARED_OBJECTS}

${STATIC_LIB}: ${SHARED_OBJECTS}
	ar rcs ${STATIC_LIB} ${SHARED_OBJECTS}

clean:
	${RM} *.obj *~* ${SHARED_OBJECTS} ${SHARED_LIB} ${STATIC_LIB} ${DEPENDENCIES}

${SHARED_OBJECTS}:
	${CPP} ${CPPFLAGS} -fPIC -o $@ ${@:%.o=%.cpp}
