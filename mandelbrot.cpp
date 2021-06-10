#include <SDL2/SDL.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>

int WINDOWSIZE = 200;
int change;

long double min = -2.0;
long double max = 2.0;
int MAX_ITERATIONS = 100;

long double map(long double value, long double in_min, long double in_max, long double out_min, long double out_max)
{
	return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

typedef struct workerBuffer
{
	int initialX;
	int initialY;
	int finalX;
	int finalY;
} worker;

typedef struct resultBuffer
{
	int X;
	int Y;
	int bright;
} result;

int main(int argc, char *argv[])
{
	int rank, size, i;
	MPI_Status status;

	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &size);

	/* create a type for struct workerBuffer */
	const int nitems = 4;
	int blocklengths[4] = {1, 1, 1, 1};
	MPI_Datatype types[4] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT};
	MPI_Datatype mpi_worker_type;
	MPI_Aint offsets[4];

	offsets[0] = offsetof(worker, initialX);
	offsets[1] = offsetof(worker, initialY);
	offsets[2] = offsetof(worker, finalX);
	offsets[3] = offsetof(worker, finalY);

	MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_worker_type);
	MPI_Type_commit(&mpi_worker_type);

	const int nitems2 = 3;
	int blocklengths2[3] = {1, 1, 1};
	MPI_Datatype types2[3] = {MPI_INT, MPI_INT, MPI_INT};
	MPI_Datatype mpi_result_type;
	MPI_Aint offsets2[3];

	offsets2[0] = offsetof(result, X);
	offsets2[1] = offsetof(result, Y);
	offsets2[2] = offsetof(result, bright);

	MPI_Type_create_struct(nitems2, blocklengths2, offsets2, types2, &mpi_result_type);
	MPI_Type_commit(&mpi_result_type);

	if (rank == 0)
	{
		worker send;
		send.initialX = 0;
		send.initialY = 0;
		send.finalX = WINDOWSIZE;
		send.finalY = WINDOWSIZE;
		MPI_Send(&send, 1, mpi_worker_type, 2, 0, MPI_COMM_WORLD);
	}
	else if (rank == 1)
	{

		SDL_Init(SDL_INIT_EVERYTHING);
		SDL_Window *window;
		SDL_Renderer *renderer;
		SDL_Event event;
		SDL_CreateWindowAndRenderer(WINDOWSIZE, WINDOWSIZE, 0, &window, &renderer);
		SDL_RenderSetLogicalSize(renderer, WINDOWSIZE, WINDOWSIZE);
		SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
		SDL_RenderClear(renderer);

		for (int i = 0; i < WINDOWSIZE * WINDOWSIZE; i++)
		{
			SDL_RenderPresent(renderer);
			result recv;
			MPI_Recv(&recv, 1, mpi_result_type, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
			//printf("X = %i, Y = %i, bright = %i", recv.X, recv.Y, recv.bright);

			SDL_SetRenderDrawColor(renderer, recv.bright, recv.bright, recv.bright, 255);
			SDL_RenderDrawPoint(renderer, recv.X, recv.Y);
		}
		usleep(10000000);
	}
	else if (rank == 2)
	{
		worker recv;
		MPI_Recv(&recv, 1, mpi_worker_type, 0, 0, MPI_COMM_WORLD, &status);

		result send;

		for (int x = recv.initialX; x < recv.finalX; x++)
		{
			for (int y = recv.initialY; y < recv.finalY; y++)
			{

				//Mapping the size of the window to (-2.0,2.0)

				long double a = map(x, 0, WINDOWSIZE, min, max); //a = x
				long double b = map(y, 0, WINDOWSIZE, min, max); //b = y

				//Mandelbrot set

				long double aInitial = a;
				long double bInitial = b;

				int count = 0;

				for (int i = 0; i < MAX_ITERATIONS; i++)
				{
					//x^2 + 2xyi - y^2
					long double a1 = a * a - b * b;
					long double b1 = 2 * a * b;

					a = a1 + aInitial;
					b = b1 + bInitial;

					if ((a + b) > 2)
					{
						break;
					}

					count++;
				}

				int bright = map(count, 0, MAX_ITERATIONS, 0, 255);

				if ((count == MAX_ITERATIONS) || (bright < 20))
				{
					bright = 0;
				}

				send.X = x;
				send.Y = y;
				send.bright = bright;
				MPI_Send(&send, 1, mpi_result_type, 1, 0, MPI_COMM_WORLD);
			}
		}
		printf("X = %i, Y = %i, bright = %i", send.X, send.Y, send.bright);
	}

	MPI_Type_free(&mpi_worker_type);
	MPI_Type_free(&mpi_result_type);
	MPI_Finalize();
	return 0;
}