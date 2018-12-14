.c.o:
	gcc -c $?

# compile node and client
all: node client

# compile node only
node: node.o util.o
	gcc -pthread -o node node.o util.o

# compile client program
client: client.o util.o
	gcc -o client client.o util.o

clean:
	rm -f *.o node client
