#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>

// These can't be const because they can be passed into the program
int WIDTH;
int HEIGHT; 

// The number of iterations before determining if a number is part of the set
int iterations = 50;
int initial_iterations = 16;

// Used to define the sizes of the color scheme arrays
#define NUMCOLORS 8

// Color palettes, based on OneDark, Nord, and Gruvbox themes
// The first color is the base color, colors after that go in
// order of how close they are to the main set
// Formatted 0xRRGGBBAA

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
	0xd8dee9ff,
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

// The currently active palette (defined in main()) 
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

Box resizeWindow;	// Automatically determined when you click somewhere on screen (measured in px)
Box currentWindow;	// The current Cartesian coordinates of the bounds of the screen

// Min and Max functions for doubles, the only thing we need it for
double min(const double a, const double b) { return (a >= b) ? b : a; }
double max(const double a, const double b) { return (a >= b) ? a : b; }

// Takes in a Cartesian coordinate pair cc and returns the
// number of iterations before f(z) > 2
int iterate(Coordinates* cc) {
	Complex_Number z = {0, 0};
	Complex_Number z_temp = z;
	Complex_Number c = {cc->x, cc->y};

	int i = 0;
	while (i < iterations && (z.real * z.real) + (z.imag * z.imag) < 4) {
		z_temp = z;

		z.real = ((z_temp.real * z_temp.real) - (z_temp.imag * z_temp.imag)) + c.real;
		z.imag = (2 * z_temp.real * z_temp.imag) + c.imag;

		i++;
	}

	return i;
}

// Change of base from pixels to Cartesian plane coordinates
Coordinates downscaleFromPixels(const Coordinates* sc) {
	Coordinates cc = {
		currentWindow.top_left.x + (sc->x / (double) WIDTH) * (currentWindow.bottom_right.x - currentWindow.top_left.x),
		currentWindow.top_left.y + (sc->y / (double) HEIGHT) * (currentWindow.bottom_right.y - currentWindow.top_left.y)
	};
	
	return cc;
}

// Takes in an SDL_Surface, fills it with Mandelbrot set pixels
void generateMandelbrotSet(SDL_Window *window, SDL_Surface *pixels, SDL_Surface *screen) {
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

			if (result == iterations) {
				row[x] = palette[0];
			}
			else {
				row[x] = palette[1 + (int)round((NUMCOLORS - 1) * result / (double)iterations)];
			}
		}
	}

	// Stop writing
	SDL_UnlockSurface(pixels);

	// Put the pixels on screen
	SDL_BlitSurface(pixels, NULL, screen, NULL);
	SDL_UpdateWindowSurface(window);
}

// Resize the window on click
void resize(SDL_Window *window, SDL_Surface *pixels, SDL_Surface *screen) {
	// Get the current window corresponding to the resize window that was drawn
	currentWindow.top_left = downscaleFromPixels(&resizeWindow.top_left);
	currentWindow.bottom_right = downscaleFromPixels(&resizeWindow.bottom_right);

	// Finally, re-draw the set
	generateMandelbrotSet(window, pixels, screen);
}

// Set the visible Cartesian window to the default values
void resetCurrentWindow() {
	currentWindow.top_left.x = -2;
	currentWindow.bottom_right.x = 1;
	currentWindow.top_left.y = -1.5;
	currentWindow.bottom_right.y = 1.5;
}

// Update the color palette by copying over all the color values to palette[]
void setPalette(const int *arr) {
	for (int i = 0; i < NUMCOLORS; i++, arr++) {
		palette[i] = *arr;
	}
}

int main(int argc, char *argv[]) {
	bool isRunning = true;
	
	// Check args
	if (argc != 1 && argc != 3) {
		SDL_LogError(SDL_LOG_PRIORITY_ERROR, "Usage: Mandelbrot.exe {WIDTH} {HEIGHT}");
	}

	// Set the dimensions
	const bool dimensionsPassedIn = (argc == 1);
	WIDTH = (dimensionsPassedIn) ? 1000 : atoi(argv[1]);
	HEIGHT = (dimensionsPassedIn) ? 1000 : atoi(argv[2]);

	// Initialize SDL stuff
	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_Window *window = SDL_CreateWindow("Mandelbrot Set Explorer", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WIDTH, HEIGHT, SDL_WINDOW_SHOWN);
	SDL_Surface *screen = SDL_GetWindowSurface(window);
	SDL_Surface *pixels = SDL_CreateRGBSurfaceWithFormat(0, WIDTH, HEIGHT, 32, SDL_PIXELFORMAT_RGBX8888);

	// Initialize window and set the default color scheme
	resetCurrentWindow();
	setPalette(PALETTE_ONEDARK);

	// Initialize all pixels to black
	SDL_FillRect(pixels, NULL, 0);

	// Actually generate the Mandelbrot Set
	generateMandelbrotSet(window, pixels, screen);

	// Event handling
	SDL_Event ev;
	while (isRunning) {
		while (SDL_PollEvent(&ev) != 0) {
			// Quit the program if the quit button is clicked
			if (ev.type == SDL_QUIT) { isRunning = false; }

			// Mouse events
			else if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.button == 1) {
				resizeWindow.bottom_right.x = min(ev.button.x + WIDTH/3, WIDTH);
				resizeWindow.bottom_right.y = min(ev.button.y + HEIGHT/3, HEIGHT);
				resizeWindow.top_left.x = max(ev.button.x - WIDTH/3, 0);
				resizeWindow.top_left.y = max(ev.button.y - HEIGHT/3, 0);

				resize(window, pixels, screen);
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
						// Reset the image size
						resetCurrentWindow();
						iterations = initial_iterations;
						generateMandelbrotSet(window, pixels, screen);
						break;
					case SDLK_i:
						// Increase iterations
						iterations = round(1.5 * iterations);
						generateMandelbrotSet(window, pixels, screen);
						break;
					case SDLK_1:
						// Set palette to OneDark
						setPalette(PALETTE_ONEDARK);
						generateMandelbrotSet(window, pixels, screen);
						break;
					case SDLK_2:
						// Set palette to Nord
						setPalette(PALETTE_NORD);
						generateMandelbrotSet(window, pixels, screen);
						break;
					case SDLK_3:
						// Set palette to Gruvbox
						setPalette(PALETTE_GRUVBOX);
						generateMandelbrotSet(window, pixels, screen);
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
