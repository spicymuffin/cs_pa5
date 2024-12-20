TARGET = pa5
CC = gcc

$(TARGET): pa5.c
	$(CC) -o $@ $^
clean:
	rm -f $(TARGET)
