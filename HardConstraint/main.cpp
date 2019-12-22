#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#pragma warning(disable:4996)
#pragma comment(linker,"/subsystem:console")

// Include SDL lib
#include "Headers/SDL2-2.0.8/include/SDL.h"
#include "Headers/SDL2-2.0.8/include/SDL_image.h"
#include "Headers/SDL2-2.0.8/include/SDL_mixer.h"
#pragma comment(lib,"Headers\\SDL2-2.0.8\\lib\\x86\\SDL2.lib")
#pragma comment(lib,"Headers\\SDL2-2.0.8\\lib\\x86\\SDL2main.lib")
#pragma comment(lib,"Headers\\SDL2-2.0.8\\lib\\x86\\SDL2.lib")
#pragma comment(lib,"Headers\\SDL2-2.0.8\\lib\\x86\\SDL2_image.lib")
#pragma comment(lib,"Headers\\SDL2-2.0.8\\lib\\x86\\SDL2_mixer.lib")

SDL_Window* window;
SDL_Renderer* renderer;
int screen_width = 800;
int screen_height = 600;


struct Vector
{
	float x;
	float y;
};

void createPoints(Vector** position, Vector** previous,  Vector** acceleration, int npoints)
{
	(*position) = (Vector*)malloc(sizeof(Vector) * npoints);
	(*previous) = (Vector*)malloc(sizeof(Vector) * npoints);
	int jump = npoints / 2;
	for (int i = 0; i < jump; i++)
	{
		(*position)[i].x = 60*i + 100;
		(*position)[i].y = 30;
		(*position)[i + jump].x = 60 * i + 100;
		(*position)[i + jump].y = 90;

		(*previous)[i].x = 60 * i + 100;
		(*previous)[i].y = 30;
		(*previous)[i + jump].x = 60 * i + 100;
		(*previous)[i + jump].y = 90;
	}
	
	*acceleration = (Vector*)malloc(sizeof(Vector) * npoints);
	memset(*acceleration, 0, sizeof(Vector) * npoints);
}

float calcDist(float x1, float y1, float x2, float y2)
{
	return sqrt((x1 - x2) * (x1 - x2) + (y1 - y2) * (y1 - y2));
}


void createConstraint(float** constraint, Vector* position, int npoints)
{
	// initialize every constraint to -1 indicating no constraint due to not connected
	for (int i = 0; i < npoints; i++)
	{
		for (int j = i; j < npoints; j++)
		{
			constraint[i][j] = -1;
			constraint[j][i] = -1;
		}
	}

	int jump = npoints / 2;
	for (int i = 1; i < npoints/2; i++)
	{
		// For vertical edges
		float distance = calcDist(position[i].x, position[i].y, position[i + jump].x, position[i + jump].y);
		constraint[i][i + jump] = distance;
		constraint[i + jump][i] = distance;

		// For horizontal edges
		distance = calcDist(position[i].x, position[i].y, position[i - 1].x, position[i - 1].y);
		constraint[i][i - 1] = distance;
		constraint[i - 1][i] = distance;
		constraint[i + jump][i + jump - 1] = distance;
		constraint[i + jump - 1][i + jump] = distance;


		// For diagonal edges
		distance = calcDist(position[i + jump].x, position[i + jump].y, position[i - 1].x, position[i - 1].y);
		constraint[i + jump][i - 1] = distance;
		constraint[i - 1][i + jump] = distance;
	}
}


void draw(Vector* position, int npoints)
{
	SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
	SDL_Rect* rect_list = (SDL_Rect*)malloc(sizeof(SDL_Rect) * npoints);
	for (int i = 0; i < npoints; i++)
	{
		rect_list[i].x = position[i].x;
		rect_list[i].y = position[i].y;
		rect_list[i].h = 7;
		rect_list[i].w = 7;
	}
	int jump = npoints / 2;
	for (int i = 1; i < npoints / 2; i++)
	{
		SDL_RenderDrawLine(renderer, position[i].x + 3, position[i].y + 3, position[i - 1].x + 3, position[i - 1].y + 3);
		SDL_RenderDrawLine(renderer, position[i + jump].x + 3, position[i + jump].y + 3, position[i + jump - 1].x + 3, position[i + jump  - 1].y + 3);
		SDL_RenderDrawLine(renderer, position[i].x + 3, position[i].y + 3, position[i + jump].x + 3, position[i + jump].y + 3);
		SDL_RenderDrawLine(renderer, position[i + jump].x + 3, position[i + jump].y + 3, position[i - 1].x + 3, position[i - 1].y + 3);
	}
	SDL_RenderFillRects(renderer, rect_list, npoints);
}

namespace physics
{
	void applyGravity(Vector* acceleration, int npoints)
	{
		for (int i = 1; i < npoints; i++)
		{
			if (i == npoints / 2) continue; // fixed point on second line
			acceleration[i].y += 0.01;
		}
	}

	void update(Vector* position, Vector* previous, Vector* acceleration, int npoints, float delta)
	{
		for (int i = 1; i < npoints; i++)
		{
			acceleration[i].x *= delta * delta;
			acceleration[i].y *= delta * delta;

			float pos_x = position[i].x * 2 - previous[i].x + acceleration[i].x;
			float pos_y = position[i].y * 2 - previous[i].y + acceleration[i].y; 
			previous[i].x = position[i].x;
			previous[i].y = position[i].y;
			position[i].x = pos_x;
			position[i].y = pos_y;
			acceleration[i].x = 0;
			acceleration[i].y = 0;
		}
	}

	void resolveConstraint(Vector* position, Vector* acceleration, float** constraint, int npoints)
	{
		int jump = npoints / 2;
		// Vertical edge
		float ver_x;
		float ver_y;
		float hor_x;
		float hor_y;
		float diag_x;
		float diag_y;
		for (int i = 1; i < npoints / 2; i++)
		{
			float x1 = position[i].x;
			float y1 = position[i].y;
			float x2 = position[i + jump].x;
			float y2 = position[i + jump].y;
			float dir_x = position[i + jump].x - position[i].x;
			float dir_y = position[i + jump].y - position[i].y;
			float length = calcDist(x1, y1, x2, y2);
			float factor = (length - constraint[i][i + jump]) / length;
			ver_x = dir_x * factor;
			ver_y = dir_y * factor;
		
			if (i == jump) continue;
			x1 = position[i].x;
			y1 = position[i].y;
			x2 = position[i - 1].x;
			y2 = position[i - 1].y;
			length = calcDist(x1, y1, x2, y2);
			dir_x = position[i - 1].x - position[i].x;
			dir_y = position[i - 1].y - position[i].y;
			factor = (length - constraint[i][i - 1]) / length;
			hor_x = dir_x * factor;
			hor_y = dir_y * factor;
			position[i].x += correct_x;
			position[i].y += correct_y;
			position[i - jump].x -= correct_x;
			position[i - jump].y -= correct_y;


			position[i].x += ver_x + hor_x;
		}

		// Diagonal edges	
		for (int i = 1; i < npoints / 2; i++)
		{
			float x1 = position[i + jump].x;
			float y1 = position[i + jump].y;
			float x2 = position[i - 1].x;
			float y2 = position[i - 1].y;
			float length = calcDist(x1, y1, x2, y2);
			float dir_x = position[i - 1].x - position[i + jump].x;
			float dir_y = position[i - 1].y- position[i + jump].y;
			float factor = (length - constraint[i + jump][i - 1]) / length;
			float correct_x = dir_x * factor;
			float correct_y = dir_y * factor;
			position[i + jump].x += correct_x;
			position[i + jump].y += correct_y;
			position[i - 1].x -= correct_x;
			position[i - 1].y -= correct_y;
		}
	}
}




int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Hard Constraint", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width, screen_height, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	Vector* position=NULL;
	Vector* previous=NULL;
	Vector* acceleration=NULL;

	int npoints = 20;
	createPoints(&position, &previous, &acceleration, npoints);
	float** constraint = (float**)malloc(sizeof(float*) * npoints);
	for (int i = 0; i < npoints; i++) constraint[i] = (float*)malloc(sizeof(float) * npoints);
	createConstraint(constraint, position, npoints);
	
	/*for (int i = 0; i < npoints; i++)
	{
		for (int j = 0; j < npoints; j++)
		{
			printf("%.2f  ", constraint[i][j]);
		}
		printf("\n");
	}
	getchar();*/
	
	unsigned int last_update = SDL_GetTicks();
	for (;;)
	{	
		SDL_Event event;	
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) return 0;
		}
		//update
		for (int step = 0; step < 15; step++)
		physics::resolveConstraint(position, acceleration, constraint, npoints);
		physics::applyGravity(acceleration,npoints);
		physics::update(position, previous, acceleration, npoints, 0.01);
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		draw(position, npoints);
		SDL_RenderPresent(renderer);
	}
	
	return 0;
}


