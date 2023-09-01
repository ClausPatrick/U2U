CC = gcc
#CFLAGS = -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -std=c11 -pthread
#CFLAGS = -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -std=c99 -pthread
CFLAGS = -Wall -Wextra -Wno-unused-variable -Wno-unused-parameter -Wno-unused-but-set-variable -std=gnu99 -pthread

SRCDIR = src
OBJDIR = obj
LOGDIR = logs

SRCS = $(wildcard $(SRCDIR)/*.c)
#OBJS = $(patsubst $(SRCDIR)/%.c,$(OBJDIR)/%.o,$(SRCS))
OBJS = obj/u2uclientdef.o obj/u2u.o obj/u2u_HAL_lx.o obj/c_logger.o obj/main.o

EXECUTABLE = u2u_HAL_lx_main

.PHONY: all clean debug

all: $(EXECUTABLE)

$(EXECUTABLE): $(OBJS)
        $(CC) $(CFLAGS) $^ -o $@

$(OBJDIR)/%.o: $(SRCDIR)/%.c
        @mkdir -p $(@D)
        $(CC) $(CFLAGS) -c $< -o $@

debug: CFLAGS += -g
debug: CFLAGS += -O0
debug: CFLAGS += -DDEBUG
debug: $(EXECUTABLE)

clean:
        rm -rf $(OBJDIR) $(EXECUTABLE)

