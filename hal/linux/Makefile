CC = gcc
CFLAGS = -Wall -Wextra

SRCDIR = src
OBJDIR = obj
LOGDIR = logs

SRCS = $(wildcard $(SRCDIR)/*.c)
OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
EXECUTABLE = u2u_HAL_lx
LOGFILES = $(wildcard $(LOGDIR)/*.txt)

.PHONY: all clean debug


# Include directories
INCLUDES = -Iinclude -pthread

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
        $(CC) $(CFLAGS) $(INCLUDES) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
        @mkdir -p $(@D)
        $(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -g
debug: CFLAGS += -O0
debug: CFLAGS += -DDEBUG
debug: $(EXECUTABLE)

clean:
        rm -rf $(OBJDIR) $(EXECUTABLE)
        rm -rf $(LOGFILES)

