#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

// Global variable used to define how big the window size should be
int WIDTH;
int HEIGHT; 
int ITERATIONS = 16;
int INITIAL_ITERATIONS;

// Color palette used to draw the set
// The earlier in the set, the lighter the color should be
#define NUMCOLORS 8

const int PALETTE_ONEDARK[NUMCOLORS] = {
	0x282c34ff,
	0xabb2bfff,
	0xe06c75ff,
	0xe5c07bff,
	0x98c379ff,
	0x56b6c2ff,
	0x61afefff,
	0xc678ddff
};

const int PALETTE_NORD[NUMCOLORS] = {
	0x2e3440ff,
	0xe5e9f0ff,
	0xa3be8cff,
	0x8fbcbbff,
	0x88c0d0ff,
	0x81a1c1ff,
	0x5e81acff,
	0xb48eadff
};

const int PALETTE_GRUVBOX[NUMCOLORS] = {
	0x282828ff,
	0xebdbb2ff,
	0xfb4934ff,
	0xfe8019ff,
	0xfabd2fff,
	0xb8bb26ff,
	0x83a598ff,
	0xd3869bff
};

int palette[NUMCOLORS]; 

typedef struct {
	double x;
	double y;
} Coordinates;

typedef struct {
	Coordinates top_left;
	Coordinates bottom_right;
} Box;

typedef struct {
	double real;
	double imag;
} Complex_Number;

// Boxes
Box resizeWindow;
Box currentWindow;

bool resizeActive = false;

// Min and Max functions
double min(const double a, const double b) {
	if (a >= b) return b;
	return a;
}

double max(const double a, const double b) {
	if (a >= b) return a;
	return b;
}

// Figure out if a given cartesian coordinate pair is in the Mandelbrot set
int iterate(Coordinates* cc) {
	Complex_Number z = {0, 0};
	Complex_Number z_temp = z;
	Complex_Number c = {cc->x, cc->y};

	int i = 0;
	while (i < ITERATIONS && (z.real * z.real) + (z.imag * z.imag) < 4) {
		z_temp = z;

		z.real = ((z_temp.real * z_temp.real) - (z_temp.imag * z_temp.imag)) + c.real;
		z.imag = (2 * z_temp.real * z_temp.imag) + c.imag;

		i++;
	}

	return i;
}

// Changes base to the smaller Cartesian plane coordinates
Coordinates downscaleFromPixels(const Coordinates* sc) {
	Coordinates cc;

	cc.x = currentWindow.top_left.x + (sc->x / (double) WIDTH) * (currentWindow.bottom_right.x - currentWindow.top_left.x);
	cc.y = currentWindow.top_left.y + (sc->y / (double) HEIGHT) * (currentWindow.bottom_right.y - currentWindow.top_left.y);
	
	return cc;
}

// Generate the Mandelbrot set into an SDL_Surface object
void generateMandelbrotSet(SDL_Surface *pixels) {
	int pitch = pixels->pitch;

	// Write to the pixels
	SDL_LockSurface(pixels);

	// Window actually displayed
	for (int y = 0; y < HEIGHT; y++) {
		unsigned int* row = (unsigned int*)((char*)pixels->pixels + pitch * y);

		for (int x = 0; x < WIDTH; x++) {
			Coordinates sc = {x, y};
			Coordinates cc = downscaleFromPixels(&sc);

			int result = iterate(&cc);

			if (result == ITERATIONS) {
				row[x] = palette[0];
			}
			else {
				row[x] = palette[1 + (int)round((NUMCOLORS - 1) * result / (double)ITERATIONS)];
			}
		}
	}

	// Stop writing
	SDL_UnlockSurface(pixels);
}

// Resize the window on second click
void resize(SDL_Surface *pixels) {
	// Get the current window corresponding to the resize window that was drawn
	currentWindow.top_left = downscaleFromPixels(&resizeWindow.top_left);
	currentWindow.bottom_right = downscaleFromPixels(&resizeWindow.bottom_right);

	// Finally, re-draw the set
	generateMandelbrotSet(pixels);
}

void resetCurrentWindow() {
	// Initialize Window values
	currentWindow.top_left.x = -2;
	currentWindow.bottom_right.x = 1;
	currentWindow.top_left.y = -1.5;
	currentWindow.bottom_right.y = 1.5;
}

void setPalette(const int *arr) {
	for (int i = 0; i < NUMCOLORS; i++, arr++) {
		palette[i] = *arr;
	}
}

// Main
int main(int argc, char *argv[]) {
	bool isRunning = true;
	
	// Check args
	if (argc != 1 && argc != 3) {
		SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Usage: Mandelbrot.exe {WIDTH} {HEIGHT}");
	}

	if (argc == 1) {
		WIDTH = 1000;
		HEIGHT = 1000;
	}
	else {
		WIDTH = atoi(argv[1]);
		HEIGHT = atoi(argv[2]);
	}

	// Initialize
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *window = SDL_CreateWindow("Mandelbrot Set Explorer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	SDL_Surface *screen = SDL_GetWindowSurface(window);
	SDL_Surface *pixels = SDL_CreateRGBSurfaceWithFormat(0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_RGBX8888);

	// Initialize window and iterations and palette
	resetCurrentWindow();
	INITIAL_ITERATIONS = ITERATIONS;
	setPalette(PALETTE_ONEDARK);

	// Set pixels to black
	SDL_FillRect(pixels, NULL, 0);

	// Actually generate the set image
	generateMandelbrotSet(pixels);

	// Put the pixels on screen
	SDL_BlitSurface(pixels, NULL, screen, NULL);
	SDL_UpdateWindowSurface(window);

	// Run the program
	SDL_Event ev;
	while (isRunning) {
		while (SDL_PollEvent(&ev) != 0) {
			// Quit the program if the quit button is clicked
			if (ev.type == SDL_QUIT) { isRunning = false; }

			// Mouse events
			else if (ev.type == SDL_MOUSEBUTTONDOWN) {
				resizeWindow.bottom_right.x = min(ev.button.x + WIDTH/3, WIDTH);
				resizeWindow.bottom_right.y = min(ev.button.y + HEIGHT/3, HEIGHT);
				resizeWindow.top_left.x = max(ev.button.x - WIDTH/3, 0);
				resizeWindow.top_left.y = max(ev.button.y - HEIGHT/3, 0);

				resize(pixels);
				SDL_BlitSurface(pixels, NULL, screen, NULL);
				SDL_UpdateWindowSurface(window);
			}

			// Keyboard events
			else if (ev.type == SDL_KEYDOWN) {
				switch (ev.key.keysym.sym) {
					case SDLK_q:
						// Stop the program (Quit)
						SDL_Log("Closing...");
						isRunning = false;
						break;
					case SDLK_p:
						// Save image as a bitmap
						SDL_Log("Saved screenshot as Mandelbrot.bmp");
						SDL_SaveBMP(pixels, "Mandelbrot.bmp");
						break;
					case SDLK_r:
						resetCurrentWindow();
						ITERATIONS = INITIAL_ITERATIONS;
						generateMandelbrotSet(pixels);
						SDL_BlitSurface(pixels, NULL, screen, NULL);
						SDL_UpdateWindowSurface(window);
						break;
					case SDLK_i:
						ITERATIONS *= 2;
						generateMandelbrotSet(pixels);
						SDL_BlitSurface(pixels, NULL, screen, NULL);
						SDL_UpdateWindowSurface(window);
						break;
					case SDLK_1:
						setPalette(PALETTE_ONEDARK);
						generateMandelbrotSet(pixels);
						SDL_BlitSurface(pixels, NULL, screen, NULL);
						SDL_UpdateWindowSurface(window);
						break;
					case SDLK_2:
						setPalette(PALETTE_NORD);
						generateMandelbrotSet(pixels);
						SDL_BlitSurface(pixels, NULL, screen, NULL);
						SDL_UpdateWindowSurface(window);
						break;
					case SDLK_3:
						setPalette(PALETTE_GRUVBOX);
						generateMandelbrotSet(pixels);
						SDL_BlitSurface(pixels, NULL, screen, NULL);
						SDL_UpdateWindowSurface(window);
						break;
				}
			}
		}
	}

	// Cleanup
	SDL_FreeSurface(screen);
	SDL_FreeSurface(pixels);
	SDL_DestroyWindow(window);
	SDL_Quit();

	return 0;
}
