#include <SDL.h>
#include <stdio.h>
#include <math.h>
#include <algorithm>
#include <string.h>
#include <future>
#include <thread>
#include <vector>
extern "C" {
	#include "rgbe.h"
};

// Constants
const int width = 1920, height = width*.5625;
const float internwidth = 2, internheight = 1.125;
float offsetx = 0, offsety = .57; float dty = -.115;
const float dt = .01;//, delta = 1;
const int iterMax = 10, iterSteps = 1, frameIters = (1<<23);
const float s0 = .5, s1 = 1, s2 = -.3, s3 = 2;
float t0 = -2, t1 = 1, t2 = 3, t3 = -4;
bool running = true;
float intensifyScreen = 4, dampenFrame = 512;
const int preRoll = 0;
const int endFrame = 2048;

char *nameStub;
SDL_Window *window;
SDL_Renderer *renderer;
SDL_Texture *texture;
Uint32 pixels[width*height];
std::vector<float*> buffers;
float frame[width*height][3];
std::vector< std::future<void> > threads;

#define XY(i, j)	((i) + (j)*width)
#define PI	3.141592654

// Velocity field control functions for x and y respectively
float f(float, float);
float g(float, float);

void popcornIterate(float*);
void insert(float*, float, float);
void preparePixels();
void prepareFrame();
void updateCoefs();
void drawScreen();
void handleEvents();
void clearData();
void quit(int);
void calc(int, float*);

int main(int argc, char **argv) {
	// Initialize SDL
	if (SDL_Init(SDL_INIT_EVERYTHING) < 0) quit(1);
	SDL_SetHint("SDL_HINT_RENDER_SCALE_QUALITY", "1");
	SDL_CreateWindowAndRenderer(std::min(width, 1280), std::min(height, 720), 0, &window, &renderer);
	if (window == NULL) quit(1);
	if (renderer == NULL) quit(1);
	SDL_SetWindowTitle(window, "Starting render...");
	texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, width, height);

	// Get name for frames
	if (argc > 1) {
		nameStub = argv[1];
	} else {
		puts("No frame saving.");
	}

	buffers.push_back(new float[width*height]);
	for (int i = 1; i < std::min(std::thread::hardware_concurrency(), 64u); i++) {
		buffers.push_back(new float[width*height]);
	}
	int threadCount = buffers.size();

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
			for (int i = 0; i < buffers.size(); i++) {
				threads.push_back(std::async(std::launch::async, calc, frameIters/threadCount/iterSteps, buffers[i]));
				total += frameIters/threadCount/iterSteps;
			}
			for (int i = 0; i < threads.size(); i++) {
				threads[i].wait();
			}
			threads.clear();

			handleEvents();
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
			fclose(img);
		}
		delta = SDL_GetTicks() - d;
		char title[512];
		sprintf(title, "Rendering on %i threads    Frame %i out of %i    Frame time: %.2f sec (%.1f%% rendering, %.1f%% display, %.1f%% saving frames)   Total time: %.2f sec    ", 
					threadCount, frameNum, endFrame, delta/1000.0, 100.0 * delta1/delta, 100.0 * delta2/delta, 100.0 - 100.0*delta3/delta, (SDL_GetTicks()-startTime)/1000.0);
		SDL_SetWindowTitle(window, title);
		clearData();
		if (frameNum >= endFrame) break;
	}
	SDL_SetWindowTitle(window, "Done");
	while (running) {
		handleEvents();
	}
	quit(0);
}

/******************************* USERS SHOULD EDIT HERE *******************************/

float f(float x, float y) {
	return cosf(t0 + y + sinf(t1 + PI * x));
}
float g(float x, float y) {
	return cosf(t2 + y + cosf(t3 + PI * x));
}

/**************************************************************************************/

void calc(int samples, float *buffer) {
	for (int total = 0; total < samples; total++) {
		popcornIterate(buffer);
	}
}

void popcornIterate(float *buffer) {
	float x = (rand()%1000000)/1000000.0; x *= 2; x -= 1; x *= internwidth;
	float y = (rand()%1000000)/1000000.0; y *= 2; y -= 1; y *= internheight;
	float dx, dy;
	for (int i = 0; i < iterMax; i++) {
		dx = f(x, y); dy = g(x, y);
		x += dx; y += dy;
		insert(buffer, x, y);
	}
}

void insert(float *buffer, float x, float y) {
	x += internwidth; x *= .5; x += offsetx; x /= internwidth; x *= width;
	y += internheight; y *= .5; y += offsety; y /= internheight; y *= height;
	int x1 = ceil(x), x0 = x1 - 1;
	int y1 = ceil(y), y0 = y1 - 1;
	float xfac = x - x0, yfac = y - y0;
	float ixfac = 1-xfac, iyfac = 1-yfac;
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
			float preval = 0;
			for (int i = 0; i < buffers.size(); i++) {
				preval += buffers[i][XY(x, y)];
			}
			pixels[XY(x, y)] = std::min(sqrt(preval)*intensifyScreen, 255.0);
		}
	}
}

void prepareFrame() {
	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			float val = 0;
			for (int i = 0; i < buffers.size(); i++) {
				val += buffers[i][XY(x, y)];
			}
			float col = sqrt(val)/dampenFrame;
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
	offsety += dty * dt;
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
	memset(pixels, 0, width*height*sizeof(Uint32));
	for (int i = 0; i < buffers.size(); i++) {
		memset(buffers[i], 0, width*height*sizeof(float));
	}
}

void quit(int rc) {
	if (rc != 0) {
		fprintf(stderr, "ERROR!\n");
	}
	SDL_Quit();
	exit(rc);
}