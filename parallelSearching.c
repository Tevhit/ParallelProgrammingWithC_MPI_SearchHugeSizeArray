//  Compile: mpicc parallelSearching.c -o searchParallel
//  Run:     mpirun -np 4 searchParallel

#include <mpi.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#define  MASTER   0
#define  MSG_START  100
#define  MSG_END   101
#define  MSG_RESULT  102
#define  MSG_DATA  103
#define  MSG_RESULT_AVERAGE 104

#define  FALSE 0
#define  TRUE 1

typedef int BOOL;

#define ARR_SIZE 900000

void* AllocateMemory(int size, const char* pcszErrMsg, BOOL bExit);
int* GenerateRandomIntegerArray(int size);
void PrintIntArray(int* pArr, int size);
int findLargestElement(int * pArr, int size);
float findAverageOfArray(int * pArr, int size);

void* AllocateMemory(int size, const char* pcszErrMsg, BOOL bExit)
{
    void* pvMem = NULL;

    pvMem = malloc(size);
    if (NULL == pvMem)
    {
        fprintf(stderr, "Cannot allocate memory : %s\n", pcszErrMsg);
        if (TRUE == bExit)
            exit(1);
    }
    return pvMem;
}

int* GenerateRandomIntegerArray(int size)
{
    int i;
    int* pArr;

    pArr = AllocateMemory(sizeof(int) * size, "Error during GenerateRandomIntegerArray", TRUE);

    for (i = 0; i < size; ++i)
        pArr[i] = rand() % 1000 + 1;
    return pArr;
}

void PrintIntArray(int* pArr, int size)
{
    int i;

    for (i = 0; i < size; ++i)
        printf("Array[%d] ..: %d\n", i, pArr[i]);
}

int findLargestElement(int * pArr, int size)
{
    int largestElement = 0;
    int i;
    for (i = 0; i < size; ++i)
    {
        if(pArr[i] > largestElement)
            largestElement = pArr[i];
    }
    return largestElement;
}

float findAverageOfArray(int * pArr, int size)
{
    float sum = 0;
    float average = 0;
    int i;
    for(i = 0; i < size; i++)
        sum += pArr[i];

    average = sum / size;
    return average;
}


int main(int argc, char** argv)
{
    int world_size;
    int process_id;
    int number;
    char* pszMsg = NULL;
    int start, end;

    srand(time(NULL));

    // Initialize the MPI environment
    MPI_Init(&argc, &argv);

    // Get the number of processes
    MPI_Comm_size(MPI_COMM_WORLD, &world_size);

    // Get the rank of the process
    MPI_Comm_rank(MPI_COMM_WORLD, &process_id);

    if (MASTER == process_id)
    {
        int items_per_slave, id;
        int* pArr;
        int slave_count;
        int largest;
        float avg;
        int result = 0;

        pArr = GenerateRandomIntegerArray(ARR_SIZE);
        //printf("Master generated a random array. It is as follows...\n");
        //PrintIntArray(pArr, ARR_SIZE);

        slave_count = (world_size - 1);
        items_per_slave = ARR_SIZE / slave_count;// -1 for MASTER

        for (id = MASTER + 1; id < world_size; ++id)
        {
            // Calculate&Send start index
            start = items_per_slave * (id - 1);
            MPI_Send(&start, 1, MPI_INT, id, MSG_START, MPI_COMM_WORLD);

            // Calculate&Send end index
            end = items_per_slave * id;
            MPI_Send(&end, 1, MPI_INT, id, MSG_END, MPI_COMM_WORLD);

            // Send Data
            MPI_Send(&pArr[start], items_per_slave, MPI_INT, id, MSG_DATA, MPI_COMM_WORLD);
        }
        float resultAverage = 0;
        for (id = MASTER + 1; id < world_size; ++id)
        {
            MPI_Recv(&largest, 1, MPI_INT, id, MSG_RESULT, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            MPI_Recv(&avg, 1, MPI_FLOAT, id, MSG_RESULT_AVERAGE, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            if(result < largest)
                result = largest;
            resultAverage += avg;
        }
        printf("---------------------\n");
        printf("The largest element of array : %d \n", result);
        printf("The average of array : %f \n", (resultAverage / slave_count));

    }
    else
    {
        int* pData;
        int item_count;
        // SLAVEs

        // Receive start index
        MPI_Recv(&start, 1, MPI_INT, MASTER, MSG_START, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //printf("Process %d received START as %d\n", process_id, start);

        // Receive end index
        MPI_Recv(&end, 1, MPI_INT, MASTER, MSG_END, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        //printf("Process %d received END as %d\n", process_id, end);

        // Receive DATA
        item_count = (end - start);
        pData = AllocateMemory(sizeof(int) * item_count, "Error during slave data memory allocation", TRUE);
        MPI_Recv(pData, item_count, MPI_INT, MASTER, MSG_DATA, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        //printf("Slave %d received data from MASTER! Theya re as follows...\n", process_id);
        //PrintIntArray(pData, item_count);

        int largest = findLargestElement(pData, item_count);
        printf("Slave %d Largest number : %d \n", process_id, largest);

        MPI_Send(&largest, 1, MPI_INT, MASTER, MSG_RESULT, MPI_COMM_WORLD);

        float avg = findAverageOfArray(pData, item_count);
        MPI_Send(&avg, 1, MPI_FLOAT, MASTER, MSG_RESULT_AVERAGE, MPI_COMM_WORLD);
    }

    // Finalize the MPI environment.
    MPI_Finalize();

    return 0;
}
