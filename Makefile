CC = gcc

all:
	${CC} -o lab1 main.c -ldl
	${CC} -o ./libs/my_lib.so ./libs/ipv4-addr.c -shared -fPIC

