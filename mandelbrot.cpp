#include <SDL2/SDL.h>
#include <mpi.h>


int WIDTH = 1000;
int HEIGHT = 1000;

long double min = -1.5;
long double max =  1.5;
int MAX_ITERATIONS = 500;
long double factor = 1;

long double map(long double value, long double in_min, long double in_max, long double out_min, long double out_max) {
	return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int main(int argc, char* argv[]) {

	SDL_Init(SDL_INIT_EVERYTHING);

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Event event;

	SDL_CreateWindowAndRenderer(1280, 720, 0, &window, &renderer);
	SDL_RenderSetLogicalSize(renderer, WIDTH, HEIGHT);

	while (true) {

		SDL_RenderPresent(renderer);

		for (int x = 0; x < WIDTH; x++) {

			if (SDL_PollEvent(&event) && event.type == SDL_QUIT)
				return 0;

			for(int y = 0; y < HEIGHT; y++){

				//Mapping the size of the window to (-2.0,2.0)

				long double a = map(x, 0, WIDTH, min, max);  //a = x
				long double b = map(y, 0, HEIGHT, min, max); //b = y

				//Mandelbrot set

				long double aInitial = a;
				long double bInitial = b;

				int count = 0;

				for (int i = 0; i < MAX_ITERATIONS; i++) {
					//x^2 + 2xyi - y^2
					long double a1 = a * a - b * b;
					long double b1 = 2 * a * b;
					
					a = a1 + aInitial;
					b = b1 + bInitial;

					if((a + b) > 2) {
						break;
					}

					count++;
				}

				int bright = map(count, 0, MAX_ITERATIONS, 0, 255);


				if ((count == MAX_ITERATIONS) || (bright < 20)) {
					bright = 0;
				}


				SDL_SetRenderDrawColor(renderer, bright, bright, bright, 255);
				SDL_RenderDrawPoint(renderer, x, y);

			}
		}
	}


	return 0;
}