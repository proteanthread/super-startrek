CC = gcc
CFLAGS = -std=c17 -Wall -O3
LDFLAGS = -lm
TARGET = startrek
SRC = startrek.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LDFLAGS)

clean:
	rm -f $(TARGET) *.o *.exe