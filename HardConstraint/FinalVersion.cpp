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
 
struct point
{
	float x;
	float y;
	float oldx;
	float oldy;
	bool alive;
};

struct stick
{
	int p1;
	int p2;
	float constraint;
};

float distance(point p1, point p2)
{
	return sqrt((p1.x - p2.x) * (p1.x - p2.x) + (p1.y - p2.y) * (p1.y - p2.y));
}

namespace physics
{
	void updatePoints(point* p, int npoints)
	{
		for (int i = 1; i < npoints; i++)
		{
			if (p[i].alive == 0) continue;
			if (i == npoints / 2) continue;
			float vx = p[i].x - p[i].oldx;
			float vy = p[i].y - p[i].oldy;
			p[i].oldx = p[i].x;
			p[i].oldy = p[i].y;
			p[i].x += vx;
			p[i].y += vy;
			p[i].y += 0.5; // gravity
		}
	}

	void updateSticks(stick* s, int nsticks, point* p, int npoints)
	{
		for (int i = 0; i < nsticks; i++)
		{
			int p1 = s[i].p1;
			int p2 = s[i].p2;
			if (p[p1].alive == 0 || p[p2].alive == 0) continue;
			float dx = p[p2].x - p[p1].x;
			float dy = p[p2].y - p[p1].y;
			float percent = (distance(p[p1], p[p2]) - s[i].constraint) / distance(p[p1], p[p2]) / 2;
			if (p1 != 0 && p1 != npoints / 2)
			{
				p[p1].x += dx * percent;
				p[p1].y += dy * percent;
			}
			if (p2 != 0 && p2 != npoints / 2)
			{
				p[p2].x -= dx * percent;
				p[p2].y -= dy * percent;
			}
		}
	}

	void constraintPoints(point* p, int npoints)
	{
		for (int i = 0; i < npoints; i++)
		{
			if (p[i].alive == 0) continue;
			float vx = p[i].x - p[i].oldx;
			float vy = p[i].y - p[i].oldy;
			if (p[i].x > screen_width)
			{
				p[i].x = screen_width;
				p[i].oldx = p[i].x + vx;
			}
			if (p[i].x < 0)
			{
				p[i].x = 0;
				p[i].oldx = p[i].x + vx;
			}
			if (p[i].y > screen_height)
			{
				p[i].y = screen_height;
				p[i].oldy = p[i].y + vy;
			}
			if (p[i].y < 0)
			{
				p[i].y = 0;
				p[i].oldy = p[i].y + vy;
			}
		}
	}

}

namespace render
{
	void renderPoints(point* p, bool* select, int npoints)
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);
		int mx, my;
		SDL_GetMouseState(&mx, &my);
		point tmp = { mx,my,0,0 };
		for (int i = 0; i < npoints; i++)
		{
			if (p[i].alive == 0) continue;
			SDL_Rect rect = { p[i].x - 3,p[i].y - 3,6,6 };
			if (distance(p[i], tmp) < 20) SDL_SetRenderDrawColor(renderer, 255, 0, 0, 255);
			else SDL_SetRenderDrawColor(renderer, 255, 255, 255, 255);
			SDL_RenderFillRect(renderer, &rect);
		}
	}

	void renderSticks(stick* s, point* p, int nsticks)
	{
		for (int i = 0; i < nsticks; i++)
		{
			int p1 = s[i].p1;
			int p2 = s[i].p2;
			if (p[p1].alive == 0 || p[p2].alive == 0) continue;
			SDL_RenderDrawLine(renderer, p[p1].x, p[p1].y, p[p2].x, p[p2].y);
		}
	}
}


void initializePoints(point* p, int npoints, stick* s)
{
	for (int i = 0; i < npoints / 2; i++)
	{
		p[i] = { (float)i * 60 + 50,30,(float)i * 60 + 50,30,1 };
		p[i + npoints / 2] = { (float)i * 60 + 50,90,(float)i * 60 + 50,90,1 };
	}
	int nsticks = 0;
	for (int i = 0; i < npoints / 2 - 1; i++)
	{
		s[nsticks++] = { i,i + 1,distance(p[i],p[i + 1]) };
		s[nsticks++] = { i + npoints / 2, i + npoints / 2 + 1, distance(p[i + npoints / 2],p[i + npoints / 2 + 1]) };
		s[nsticks++] = { i + 1, i + npoints / 2 + 1, distance(p[i + 1],p[i + npoints / 2 + 1]) };
		s[nsticks++] = { i,i + npoints / 2 + 1,distance(p[i],p[i + npoints / 2 + 1]) };
	}
}



int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_VIDEO);
	window = SDL_CreateWindow("Hard Constraint", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, screen_width, screen_height, SDL_WINDOW_SHOWN);
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);

	int npoints = 20;
	point* p = (point*)malloc(sizeof(point) * npoints);
	int nsticks = 2 * npoints - 4;
	stick* s = (stick*)malloc(sizeof(stick) * nsticks);
	initializePoints(p, npoints, s);
	bool* select = (bool*)malloc(sizeof(bool) * npoints);

	int lastframe = SDL_GetTicks();
	for (;;)
	{
		memset(select, 0, sizeof(bool) * npoints);
		SDL_Event event;
		while (SDL_PollEvent(&event))
		{
			if (event.type == SDL_QUIT) return 0;
			if (event.type == SDL_MOUSEBUTTONDOWN)
			{
				int mx, my;
				SDL_GetMouseState(&mx, &my);
				point tmp = { mx,my,0,0 };
				for (int i = 0; i < npoints; i++)
				{
					if (distance(p[i], tmp) < 20) p[i].alive = 0;
				}
			}
		}
		//update
		int time = SDL_GetTicks();
		if (time - lastframe < 30) continue;
		lastframe = time;
		physics::updatePoints(p, npoints);
		
		for (int step = 0; step < 15; step++)
		{
			physics::updateSticks(s, nsticks, p, npoints);
			//physics::constraintPoints(p, npoints);
		}
		render::renderPoints(p, select, npoints);
		render::renderSticks(s, p, nsticks);
		SDL_RenderPresent(renderer);
	}

	return 0;
}


