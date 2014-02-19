#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>

const int width = 640, height = 480;
const double internwidth = 2, internheight = 1.5;
const double offsetx = 0, offsety = .25;
const double delta = 1;
const int iterMax = 10;
bool running = true;

SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
Uint32 pixels[width*height];
double buffer[width*height];

#define XY(i, j)	((i) + (j)*width)
#define PI	3.141592654

double f(double x, double y) {
	return cos(0 + y + sin(1 + PI * x));
}
double g(double x, double y) {
	return cos(2 + y + cos(3 + PI * x));
}

void popcornIterate();
void insert(double, double);
void preparePixels();
void drawScreen();
void handleEvents();
void quit(int);

int main() {
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) quit(1);
	SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
	if (window == NULL) quit(1);
	if (renderer == NULL) quit(1);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	int total = 0;
	while (running) {
		long a = SDL_GetTicks();
		for (int i = 0; i < 1000; i++) {
			popcornIterate();
			handleEvents();
			total++;
		}
		long b = SDL_GetTicks();
		preparePixels();
		drawScreen();
		long c = SDL_GetTicks();
		long delta1, delta2, delta;
		delta = c - a; delta1 = b - a; delta2 = c - b;
		printf("\rdt for iteration is %.2f seconds, %.2f%% in calculation, %.2f%% in drawing.", delta/1000.0, 100.0 * delta1/delta, 100.0 * delta2/delta);
		fflush(stdout);
	}
	quit(0);
}

void popcornIterate() {
	double x = (rand()%1000000)/1000000.0; x *= 2; x -= 1; x *= internwidth;
	double y = (rand()%1000000)/1000000.0; y *= 2; y -= 1; y *= internheight;
	double dx, dy;
	for (int i = 0; i < iterMax; i++) {
		dx = f(x, y); dy = g(x, y);
		x += delta * dx;
		y += delta * dy;
		insert(x, y);
	}
}

void insert(double x, double y) {
	x += internwidth; x *= .5; x += offsetx; x /= internwidth; x *= width;
	y += internheight; y *= .5; y += offsety; y /= internheight; y *= height;
	int x1 = ceil(x), x0 = x1 - 1;
	int y1 = ceil(y), y0 = y1 - 1;
	double xfac = x - x0, yfac = y - y0;
	double ixfac = 1-xfac, iyfac = 1-yfac;
	if (y0 >= 0 && x0 >= 0 && y1 < height && x1 < width) {
		buffer[XY(x0, y0)] += ixfac * iyfac;
		buffer[XY(x1, y0)] += xfac * iyfac;
		buffer[XY(x0, y1)] += ixfac * yfac;
		buffer[XY(x1, y1)] += xfac * yfac;
	}
}

void preparePixels() {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			pixels[XY(x, y)] = std::min((int) sqrt(buffer[XY(x, y)])*4, 255);
		}
	}
}

void drawScreen() {
	SDL_UpdateTexture(texture, NULL, pixels, width * sizeof(Uint32));
	SDL_RenderClear(renderer);
	SDL_RenderCopy(renderer, texture, NULL, NULL);
	SDL_RenderPresent(renderer);
}

void handleEvents() {
	SDL_Event event;
	while (SDL_PollEvent(&event)) {
		switch (event.type) {
			case SDL_QUIT:
				running = false;
				break;
			case SDL_KEYDOWN:
				if (event.key.keysym.sym == SDLK_ESCAPE) {
					running = false;
				}
				break;
		}
	}
}

void quit(int rc) {
	if (rc != 0) {
		fprintf(stderr, "ERROR!\n");
	}
	SDL_Quit();
	exit(rc);
}