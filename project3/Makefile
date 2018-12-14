.c.o:
	gcc -g -c $?

# compile proxy
all: proxy

# compile proxy
proxy: proxy.o utils.o
	gcc -g -o proxy proxy.o utils.o

clean:
	rm -f *.o proxy
