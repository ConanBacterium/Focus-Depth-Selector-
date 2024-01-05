# Define the compiler
CC=gcc

# Define any compile-time flags
CFLAGS=-Wall -g

# Define any directories containing header files
INCLUDES=-I/usr/local/include/opencv4

# Define library paths and libraries
LFLAGS=-L/usr/local/lib
LIBS=-lopencv_core -lopencv_highgui -lopencv_imgcodecs -lopencv_imgproc

# Define the C source files
SRCS=dynamic_focus.c

# Define the C object files
OBJS=$(SRCS:.c=.o)

# Define the executable file
MAIN=dynfoc

all: $(MAIN)
	@echo Program compiled

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS)

.c.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	$(RM) *.o *~ $(MAIN)
