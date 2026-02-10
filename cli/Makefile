#
# Makefile for tuner_cli
#

CC = clang
CFLAGS = -O2 -Wall -framework Accelerate
TARGET = tuner_cli

$(TARGET): tuner_cli.c
	$(CC) $(CFLAGS) -o $(TARGET) tuner_cli.c

clean:
	rm -f $(TARGET)

.PHONY: clean
