#include <SDL2/SDL.h>
#include <mpi.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdio.h>

const int NUM_THREADS_REQUIRED = 2;
const bool DEBUG_CONSOLE = false;
const char EXT[5] = ".buf";

int windowSizeX = 800;
int windowSizeY = 800;
int maxInterations = 100;

long double min = -2.0;
long double max = 2.0;

bool sleepThread = false;

typedef struct workerBuffer
{
    int initialX;
    int initialY;
    int finalX;
    int finalY;
    int rank;
} worker;

long double map(long double value, long double in_min, long double in_max, long double out_min, long double out_max)
{
    return (value - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int maxDiv(int value)
{
    int valueC = value - 1;
    while (value % valueC != 0 && valueC > 1)
    {
        valueC--;
    }
    if (valueC == 1)
    {
        return value;
    }
    return valueC;
}

void getName(char* name, int rank)
{
    sprintf(name, "%d%s", rank, EXT);
}

void divThread(int sizeReal, MPI_Datatype sendType)
{
    float columnCount = sqrt(sizeReal);
    int rowCount = (int) columnCount;

    //NAO E RAIZ EXATA
    if (columnCount != rowCount)
    {
        columnCount = maxDiv(sizeReal);
        rowCount = sizeReal / (int) columnCount;
    }

    int lengthX = (int) windowSizeX / columnCount;
    int lengthDiffX = windowSizeX - (lengthX * columnCount);
    int lengthY = (int) windowSizeY / rowCount;
    int lengthDiffY = windowSizeY - (lengthY * rowCount);
    int initialX = 0;
    int initialY = 0;
    int finalX = lengthX;
    int finalY = lengthY;

    bool useLengthDiffX = lengthDiffX > 0;
    bool useLengthDiffY = lengthDiffY > 0;

    if (DEBUG_CONSOLE)
    {
        printf ("LENGTH X %d\n", lengthX);
        printf ("LENGTH DIF X %d\n", lengthDiffX);
        printf ("COLUMN COUNT %f.2\n", columnCount);
        printf ("ROW COUNT %d\n", rowCount);
    }

    int j = 1;
    for (int i = NUM_THREADS_REQUIRED; i < sizeReal + NUM_THREADS_REQUIRED; i++)
    {
        bool resetInitialX = (i - 1) % ((int) columnCount) == 0;

        if (useLengthDiffX && resetInitialX)
        {
            finalX += lengthDiffX;
        }

        worker send;
        send.initialX = initialX;
        send.initialY = initialY;
        send.finalX = finalX;
        send.finalY = finalY;
        send.rank = i;

        MPI_Send(&send, 1, sendType, i, 0, MPI_COMM_WORLD);

        //FAZ O PROCESSAMENTO DAS NOVAS POSICOES PARA A PROXIMA THREAD
        if (resetInitialX)
        {
            j++;
        }

        bool lastTY = j == rowCount;

        if (DEBUG_CONSOLE)
        {
            printf ("RANK %d\n", i);
            printf ("INITIAL X %d\n", initialX);
            printf ("INITIAL Y %d\n", initialY);
            printf ("FINAL X %d\n", finalX);
            printf ("FINAL Y %d\n", finalY);
        }

        //ATRIBUI OS VALORES INICIAIS CORRETOS PARA A PROXIMA THREAD
        if (resetInitialX)
        {
            initialX = 0;
            initialY = finalY;
        }
        else
        {
            initialX = finalX;
        }

        //CALCULO DO TAMANHO NO EIXO X PARA A PROXIMA THREAD
        finalX += lengthX;
        if (resetInitialX)
        {
            finalY += lengthY;
            finalX = lengthX;
        }

        //CALCULO DO TAMANHO NO EIXO Y PARA A PROXIMA THREAD
        if (lastTY && useLengthDiffY)
        {
            finalY = lengthY * rowCount + lengthDiffY;
        }
    }
}

void printWindow(int sizeReal)
{
    SDL_Init(SDL_INIT_EVERYTHING);
    SDL_Window *window;
    SDL_Renderer *renderer;
    SDL_Event event;
    SDL_CreateWindowAndRenderer(windowSizeX, windowSizeY, 0, &window, &renderer);
    SDL_RenderSetLogicalSize(renderer, windowSizeX, windowSizeY);
    SDL_SetRenderDrawColor(renderer, 200, 200, 200, 255);
    SDL_RenderClear(renderer);
    MPI_Status status;
    for (int i = 0; i < sizeReal; i++)
    {
        SDL_RenderPresent(renderer);
        int recv;
        char name[100];
        MPI_Recv(&recv, 1, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &status);
        getName(name, recv);
        FILE *fBuffer = fopen(name, "r");
        size_t len = 100;
        char *line = (char *) malloc(len);
        while (getline(&line, &len, fBuffer) > 0)
        {
            int x = atoi(line);
            getline(&line, &len, fBuffer);
            int y = atoi(line);
            getline(&line, &len, fBuffer);
            int brigth = atoi(line);
            SDL_SetRenderDrawColor(renderer, brigth, brigth, brigth, 255);
            SDL_RenderDrawPoint(renderer, x, y);
        }
        remove(name);
    }
    SDL_RenderPresent(renderer);
    usleep(10000000);
}

void slave(MPI_Datatype recType)
{
    MPI_Status status;
    worker recv;
    MPI_Recv(&recv, 1, recType, 0, 0, MPI_COMM_WORLD, &status);
    char name[100];
    getName(name, recv.rank);
    FILE *fBuffer = fopen(name, "w+");
    if (DEBUG_CONSOLE)
    {
        printf ("RANK - CALCULO %d\n", recv.rank);
        printf ("INITIAL X %d\n", recv.initialX);
        printf ("INITIAL Y %d\n", recv.initialY);
        printf ("FINAL X %d\n", recv.finalX);
        printf ("FINAL Y %d\n", recv.finalY);
    }

    for (int x = recv.initialX; x < recv.finalX; x++)
    {
        for (int y = recv.initialY; y < recv.finalY; y++)
        {

            //Mapping the size of the window to (-2.0,2.0)

            long double a = map(x, 0, windowSizeX, min, max); //a = x
            long double b = map(y, 0, windowSizeY, min, max); //b = y

            //Mandelbrot set

            long double aInitial = a;
            long double bInitial = b;

            int count = 0;

            for (int i = 0; i < maxInterations; i++)
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

            int bright = map(count, 0, maxInterations, 0, 255);

            if ((count == maxInterations) || (bright < 20))
            {
                bright = 0;
            }
            fprintf(fBuffer, "%d\n", x);
            fprintf(fBuffer, "%d\n", y);
            fprintf(fBuffer, "%d\n", bright);
        }
    }
    fclose(fBuffer);
    if (sleepThread && recv.rank % 2 == 0)
    {
        usleep(10000000);
    }
    MPI_Send(&recv.rank, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);
}
int main(int argc, char *argv[])
{
    int rank, size, sizeReal, i;
    MPI_Status status;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    for(int i = 1; i < argc; i++)
    {
        char *param = argv[i];
        if (strcmp(param, "-s") == 0)
        {
            sleepThread = true;
        }
        else if (strcmp(param, "-w") == 0)
        {
            windowSizeX = atoi(argv[++i]);
        }
        else if (strcmp(param, "-h") == 0)
        {
            windowSizeY = atoi(argv[++i]);
        }
        else if (strcmp(param, "-i") == 0)
        {
            maxInterations = atoi(argv[++i]);
        }
    }

    sizeReal = size - NUM_THREADS_REQUIRED;

    if (sizeReal <= 1)
    {
        printf("Numero minimo (%d) de threads nao informado.\n", NUM_THREADS_REQUIRED);
        MPI_Finalize();
        return 1;
    }

    /* create a type for struct workerBuffer */
    const int nitems = 5;
    int blocklengths[5] = {1, 1, 1, 1, 1};
    MPI_Datatype types[5] = {MPI_INT, MPI_INT, MPI_INT, MPI_INT, MPI_INT};
    MPI_Datatype mpi_worker_type;
    MPI_Aint offsets[5];

    offsets[0] = offsetof(worker, initialX);
    offsets[1] = offsetof(worker, initialY);
    offsets[2] = offsetof(worker, finalX);
    offsets[3] = offsetof(worker, finalY);
    offsets[4] = offsetof(worker, rank);

    MPI_Type_create_struct(nitems, blocklengths, offsets, types, &mpi_worker_type);
    MPI_Type_commit(&mpi_worker_type);

    switch(rank)
    {
    case 0:
        divThread(sizeReal, mpi_worker_type);
        break;
    case 1:
        printWindow(sizeReal);
        break;
    default:
        slave(mpi_worker_type);
        break;
    }

    MPI_Type_free(&mpi_worker_type);
    MPI_Finalize();
    return 0;
}
