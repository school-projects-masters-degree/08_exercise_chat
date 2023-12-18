CC = gcc
# Compile chat.c flags
CFLAGS = -g

chat: chat.o
	$(CC) $(CFLAGS) -o chat chat.o