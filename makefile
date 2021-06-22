all: pi pi-example

pi: pi.c
	gcc -g -pthread -O0 pi.c -o pi -lrt

pi-example: pi-example.c
	gcc -g -pthread -O0 pi-example.c -o pi-example

clean:
	rm pi pi-example