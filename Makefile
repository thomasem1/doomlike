build:
	gcc -o mygame src/main.c -lSDL2 -lm

run:
	./mygame

all: build run

clean:
	rm mygame
