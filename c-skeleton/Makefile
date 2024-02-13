CFLAGS=-g -O2 -Wall -Wextra -Isrc -rdynamic -I/usr/local/include/opencv4 -DNDEBUG $(OPTFLAGS)
LIBS=-ldl -lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc  $(OPTLIBS)
PREFIX?=/usr/local 

SOURCES=$(wildcard src/**/*.c src/*.c) 
HEADERS=$(wildcard src/**/*.h src/*.h)
OBJECTS=$(patsubst %.c,%.o,$(SOURCES))

TEST_SRC=$(wildcard tests/*_tests.c) 
TESTS=$(patsubst %.c, %, $(TEST_SRC)) 

PROGRAMS_SRC=$(wildcard bin/*.c)
PROGRAMS=$(patsubst %.c,%,$(PROGRAMS_SRC))

TARGET=build/libDynFoc.a
SO_TARGET=$(patsubst %.a,%.so,$(TARGET))

# The Target Build
all: $(TARGET) $(SO_TARGET) tests $(PROGRAMS)

dev: CFLAGS=-g -Wall -Isrc -Wall -Wextra $(OPTFLAGS) 
dev: all 

$(TESTS): $(TARGET) $(LIBS) $(SO_TARGET)

$(TARGET): CFLAGS += -fPIC
$(TARGET): build $(OBJECTS) 
	ar rcs $@ $(OBJECTS)
	ranlib $@
	
$(SO_TARGET): $(TARGET) $(OBJECTS)
	$(CC) $(LDFLAGS) -shared -o $@ $(OBJECTS) 
	
build: 
	@mkdir -p build
	@mkdir -p bin 
	
# The Unit Tests 
.PHONY: tests 
tests: CFLAGS += $(TARGET)
tests: $(TESTS) 
	sh ./tests/runtests.sh
	
valgrind: 
	VALGRIND="valgrind --log-file=/tmp/valgrind-%p.log" $(MAKE)

# The Cleaner 
clean: 
	rm -rf build $(OBJECTS) $(TESTS) $(PROGRAMS) 
	rm -f tests/tests.log 
	find . -name "*.gc*" -exec rm {} \; 
	rm -rf `find . -name "*.dSYM" -print`
	
# The Install
install: all
	install -d $(DESTDIR)/$(PREFIX)/lib/
	install $(TARGET) $(DESTDIR)/$(PREFIX)/lib/
	install $(SO_TARGET) $(DESTDIR)/$(PREFIX)/lib/
	#install -d $(DESTDIR)/$(PREFIX)/include/lcthw
	#install -d $(DESTDIR)/$(PREFIX)/include/lcthw
	
# The Checker 
BADFUNCS='[^_.>a-zA-Z0-9](str(n?cpy|n?cat|xfrm|n?dup|str|pbrk|tok|_)\|stpn?cpy|a?sn?printf|byte_)'
check: 
	@echo Files with potentially dangerous functions. 
	@egrep $(BADFUNCS) $(SOURCES) || true 
$(PROGRAMS): CFLAGS += $(TARGET)