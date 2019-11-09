CC = gcc

CFLAGS = -Wall
CFLAGS += `pkg-config --cflags --libs vorbisfile`

ogg2mogg.exe: main.c
	$(CC) -o ogg2mogg main.c $(CFLAGS)
