CC = gcc
CFLAGS = -g -O2 -Wall -pedantic
OBJECTS = mailout.o smtp.o message.o

main : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o mailout

%.o : %.c
	$(CC) $(CFLAGS) -c $<
