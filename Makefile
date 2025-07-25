CC = gcc
CFLAGS = -Wall -I/opt/homebrew/include
LIBS = -L/opt/homebrew/lib -lltc -lportaudio -lm

TARGET = ltc_live
SRC = ltc_live.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC) $(LIBS)

clean:
	rm -f $(TARGET)
