CC = gcc
CFLAGS = -Wall -Wextra -std=c99
LIBS = `pkg-config --cflags --libs jansson`
TARGET = grand_prixdictor
SOURCE = grand_prixdictor.c

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(SOURCE)
	$(CC) $(CFLAGS) $(LIBS) $(SOURCE) -o $(TARGET)

clean:
	rm -f $(TARGET) $(TARGET).o

install:
	@echo "Installing jansson dependency..."
	@which brew > /dev/null && brew install jansson || echo "Please install jansson library manually"

.PHONY: install