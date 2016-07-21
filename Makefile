CC   	= gcc 
CFLAGS  = -I.
LIBS  	= -lpthread
OBJS  	= manifest.o server.o client.o 

all: $(OBJS)
	$(CC) client.o manifest.o -o client $(CFLAGS) $(LIBS)
	$(CC) server.o manifest.o -o server $(CFLAGS)

manifest.o: manifest.c manifest.h
	$(CC) -c  $< -o $@ $(CFLAGS)

server.o: server.c server.h manifest.h
	$(CC) -c  $< -o $@ $(CFLAGS)

client.o: client.c client.h manifest.h
	$(CC) -c  $< -o $@ $(CFLAGS)

clean: 
	rm *.o client server