.PHONY: clean test
.DEFAULT_GOAL := kv

kv.o: kv.c
	gcc -c kv.c -o kv.o 

kv: main.c kv.h kv.o
	gcc -o kv main.c kv.o

test: kv
	./test-kv.sh

clean:
	rm kv *.o
