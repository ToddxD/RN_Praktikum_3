# Compiler und Flags
CC = gcc
CFLAGS = -Wall -Wno-multichar -std=gnu17

# Ziele (Executables)
TARGET = build/network_app

# Quell- und Objektdateien
SOURCEDIR = src
SRCS := $(shell find $(SOURCEDIR) -name '*.c')
OBJS = $(SRCS:.c=.o)

# Standard-Target: baut beide Programme
all: $(TARGET)

# --- Server ---
$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Kompilierschritt für einzelne .c-Dateien
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Aufräumen
clean:
	rm -f $(OBJS) $(TARGET)
