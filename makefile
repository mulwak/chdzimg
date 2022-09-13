CC := gcc

bin/bmp2nibble: src/bmp2nibble.c
	$(CC) src/bmp2nibble.c -o bin/bmp2nibble

