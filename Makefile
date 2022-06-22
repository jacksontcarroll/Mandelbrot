CC = gcc
CFLAGS = -lm -lSDL2

Mandelbrot.exe: main.c
	$(CC) main.c $(CFLAGS) -o Mandelbrot.exe

clean:
	rm -rf Mandelbrot.exe Mandelbrot.bmp
