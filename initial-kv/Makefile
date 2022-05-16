.PHONY: clean test
.DEFAULT_GOAL := kv

kv: main.c
	gcc -o kv main.c

test: kv
	./test-kv.sh

clean:
	rm kv
