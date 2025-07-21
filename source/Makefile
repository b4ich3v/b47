CC = gcc
CFLAGS = -Wall -Wextra -std=c11 -g
LDLIBS = -lreadline

SRCS = shell.c builtins.c process.c monitor.c \
       lexer.c parser.c executor.c expand.c variable.c
OBJS = $(SRCS:.c=.o)

all: b47

b47: $(OBJS)
	$(CC) $(CFLAGS) $^ -o $@ $(LDLIBS)

clean:
	rm -f b47 $(OBJS)
