# Compiler und Flags
CC = gcc
CFLAGS = -Wall -Wno-multichar -std=gnu17 -g -pthread

# Ziele (Executables)
TARGET = $(OBJDIR)/network_app

# Quell- und Objektdateien
SOURCEDIR = src
OBJDIR = build
SRCS := $(shell find $(SOURCEDIR) -name '*.c')
OBJS = $(SRCS:$(SOURCEDIR)/%.c=$(OBJDIR)/%.o)

all: $(TARGET)


$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

# Kompilierschritt für einzelne .c-Dateien
$(OBJDIR)/%.o: $(SOURCEDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

$(OBJDIR):
	mkdir -p $(OBJDIR)

# Aufräumen
clean:
	rm -f $(OBJS) $(TARGET)
