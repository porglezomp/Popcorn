#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <string.h>
extern "C" {
	#include "rgbe.h"
};

// Constants
const int width = 1280, height = 720;
const double internwidth = 2, internheight = 1.125;
double offsetx = 0, offsety = .57; double dty = -.0023;
const double delta = 1, dt = .02;
const int iterMax = 10, iterStep = (1<<19), frameIters = (1<<19);
const double s0 = .5, s1 = 1, s2 = -.3, s3 = 2;
double t0 = -2, t1 = 1, t2 = 3, t3 = -4;
bool running = true;
float intensifyScreen = 16, dampenFrame = 128;
const int preRoll = 0;
const int endFrame = 1024;

char *nameStub;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
Uint32 pixels[width*height];
double buffer[width*height];
float frame[width*height][3];

#define XY(i, j)	((i) + (j)*width)
#define PI	3.141592654

// Velocity field control functions for x and y respectively
double f(double, double);
double g(double, double);

void popcornIterate();
void insert(double, double);
void preparePixels();
void prepareFrame();
void updateCoefs();
void drawScreen();
void handleEvents();
void clearData();
void quit(int);

int main(int argc, char **argv) {
	// Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) quit(1);
	SDL_CreateWindowAndRenderer(width, height, 0, &window, &renderer);
	if (window == NULL) quit(1);
	if (renderer == NULL) quit(1);
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);
	
	// Get name for frames
	if (argc > 1) {
		nameStub = argv[1];
	} else {
		puts("No frame saving.");
	}

	long startTime = SDL_GetTicks();
	// Pre-roll
	int frameNum = preRoll;
	for (int i = 0; i < frameNum; i++) {
		updateCoefs();
	}
	while (running) {
		frameNum++;
		long delta1 = 0, delta2 = 0, delta3 = 0, delta = 0;
		long d = SDL_GetTicks();
		for (int total = 0; total < frameIters;) {
			long a = SDL_GetTicks();
			for (int i = 0; i < iterStep; i++) {
				popcornIterate();
				handleEvents();
				total++;
			}
			long b = SDL_GetTicks();
			preparePixels();
			drawScreen();
			
			// Debug time output
			long c = SDL_GetTicks();
			delta1 += b - a; delta2 += c - b; delta3 += c - a; 
			if (!running) break;
		}
		// Change the function coefficients for animation
		updateCoefs();
		// Frame output
		if (running && argc > 1) {
			prepareFrame();
			char name[1024];
			sprintf(name, "%s%i.hdr", nameStub, frameNum);
			FILE *img = fopen(name, "wb");
			RGBE_WriteHeader(img, width, height, NULL);
			RGBE_WritePixels_RLE(img, (float *) frame, width, height);
		}
		delta = SDL_GetTicks() - d;
		printf("Total deltatime for frame %i is %.2f seconds, %.2f%% in calculation, %.2f%% in drawing, %.2f%% in frame output. Total elapsed time is %.2f seconds.\n",
					frameNum, delta/1000.0, 100.0 * delta1/delta, 100.0 * delta2/delta, 100.0 - 100.0*delta3/delta, (SDL_GetTicks()-startTime)/1000.0);

		clearData();
		if (frameNum >= endFrame) running = false;
	}
	quit(0);
}

/******************************* USERS SHOULD EDIT HERE *******************************/

double f(double x, double y) {
	return cos(t0 + y + sin(t1 + PI * x));
}
double g(double x, double y) {
	return cos(t2 + y + cos(t3 + PI * x));
}

/**************************************************************************************/

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
			pixels[XY(x, y)] = std::min(sqrt(buffer[XY(x, y)])*intensifyScreen, 255.0);
		}
	}
}

void prepareFrame() {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float col = sqrt(buffer[XY(x, y)])/dampenFrame;
			float* pixel = frame[XY(x, y)];
			pixel[0] = col; pixel[1] = col; pixel[2] = col;
		}
	}
}

void updateCoefs() {
	t0 += s0 * dt;
	t1 += s1 * dt;
	t2 += s2 * dt;
	t3 += s3 * dt;
	offsety += dty;
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

void clearData() {
	memset(frame, 0, width*height*3*sizeof(float));
	memset(buffer, 0, width*height*sizeof(double));
	memset(pixels, 0, width*height*sizeof(Uint32));
}

void quit(int rc) {
	if (rc != 0) {
		fprintf(stderr, "ERROR!\n");
	}
	SDL_Quit();
	exit(rc);
}