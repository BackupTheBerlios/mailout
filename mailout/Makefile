CC = gcc
CFLAGS = -g -O2 -Wall -pedantic
OBJECTS = recipient.o smtp.o message.o queue.o mailout.o

main : $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o mailout

%.o : %.c
	$(CC) $(CFLAGS) -c $<

clean:
	/bin/rm -f *.o mailout *.core

