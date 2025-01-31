CC = gcc
CXX = g++
#CFLAGS = -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -std=c11 -pthread 
CFLAGS = -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -std=gnu99 -pthread 
CXXFLAGS = -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -std=c++17 -pthread 

SRCDIR = src
OBJDIR = obj
LOGDIR = logs

SRCC = $(wildcard $(SRCDIR)/**/*.c)
SRCXX = $(wildcard $(SRCDIR)/**/*.cpp)
OBJXX = obj/main.o
OBJC = obj/u2u.o obj/u2u_HAL_pico.o obj/c_logger.o obj/hardware/uart.o obj/hardware/gpio.o

LOGFILES = $(wildcard $(LOGDIR)/*.txt)

EXECUTABLE = u2u_tester

.PHONY: all clean debug

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJC) $(OBJXX)
	@echo 'objc1: $(OBJC)'
	@echo 'objxx1: $(OBJXX)'
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
	@mkdir -p $(@D)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(@D)
	$(CXX) $(CXXFLAGS) -c $< -o $@


debug: CFLAGS += -g
debug: CFLAGS += -O0
debug: CFLAGS += -DDEBUG
debug: $(EXECUTABLE)

clean:
	rm -rf $(OBJDIR) $(EXECUTABLE)
	rm -rf $(LOGFILES)

