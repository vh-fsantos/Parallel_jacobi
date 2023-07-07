#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>

#define SIZE 10000

//---------------------------------------------------------

struct jacobiResult{
    int iterations;
    double values[SIZE];
    int converged;
} typedef JACOBI;

//---------------------------------------------------------

double norma_vector(double *x){
    double sum = 0;
    for (int i = 0; i < SIZE; i++){
        sum += pow(x[i], 2);
    }

    return sqrt(sum);
}

//---------------------------------------------------------

void update_x(int start, int np, double* x, double* new_x, double** matrix, double* constants)
{
    for (int i = start; i < SIZE; i += np)
    {
        double sum = 0;
        for (int j = 0; j < SIZE; j++) 
        {
            if (j != i) 
            {
                sum += matrix[i][j] * x[j];
            }
        }

        new_x[i] = (constants[i] - sum) / matrix[i][i];
    }
}

//---------------------------------------------------------

JACOBI jacobi(double** matrix, double* constants, int maxIterations, double error, int np) 
{
    double* x; // Representa as tentativas
    double* new_x; // Vetor com o resultado final
    JACOBI result;

    // Criando área de memória compartilhada para o valor de X
	int shmid_x = shmget(7, SIZE * sizeof(double), 0600 | IPC_CREAT);
	int shmid_new_x = shmget(8, SIZE * sizeof(double), 0600 | IPC_CREAT);
    x = (double *) shmat(shmid_x, NULL, 0);
    new_x = (double *) shmat(shmid_new_x, NULL, 0);
    // Inicializando o X
    for (int i = 0; i < SIZE; i++)
    {
        x[i] = 0;   
    }

    // Algoritmo do jacobi - resolver o sistema
    int k = 0;
    while (k < maxIterations) 
    {
        int pid;
        // Paralelizando a atualização do X
        for (int i = 0; i < np; i++)
        {
            pid = fork();
            
            if (pid == 0)
            {
                update_x(i, np, x, new_x, matrix, constants);
                exit(0);
            }
        }
        // Esperanod processos filhos
        if (pid > 0)
        {
            for (int i = 0; i < np; i++)
            {
                wait(NULL);
            }
        }

        // Verificando a convergencia
        if (fabs(norma_vector(x) - norma_vector(new_x)) < error)
        {
            result.iterations = k + 1;
            result.converged = 1;
            for (int i = 0; i < SIZE; i++)
            {
                result.values[i] = new_x[i];
            }

            shmdt(x);
            shmdt(new_x);
            shmctl(shmid_x, IPC_RMID, 0);
            shmctl(shmid_new_x, IPC_RMID, 0);

            return result;
        }
        else
        { // Atualizando o vetor de tentativas com o valor corrente do de resultado
            for (int i = 0; i < SIZE; i++)
            {
                x[i] = new_x[i];
            }
        }
        k++;
    }
    
    shmdt(x);
    shmdt(new_x);
    shmctl(shmid_x, IPC_RMID, 0);
    shmctl(shmid_new_x, IPC_RMID, 0);

    return result;
}

//---------------------------------------------------------

double** createMatrix() 
{
    double** matrix = (double**) malloc(SIZE * sizeof(double*));

    for (int i = 0; i < SIZE; i++) 
    {
        matrix[i] = (double*) malloc(SIZE * sizeof(double));
    }

    for (int i = 0; i < SIZE; i++) 
    {
        for (int j = 0; j < SIZE; j++) 
        {
            matrix[i][j] = 0;
        }
    }

    for (int i = 0; i < SIZE; i++)
    {
        matrix[i][i] = 6;
    }

    for (int i = 0; i < SIZE - 1; i++) 
    {
        matrix[i][i + 1] = 1;
        matrix[i + 1][i] = 1;
    }

    return matrix;
}

//---------------------------------------------------------

double* createArray() {
    double* array = (double*) malloc(SIZE * sizeof(double));
    int middle_index = SIZE / 2;

    for (int i = 0; i <= middle_index; i++) {
        array[middle_index - i] = i;
        array[middle_index + i] = i;
    }

    array[middle_index] = -6;

    return array;
}

//---------------------------------------------------------

int main(int argc, char** argv) 
{
    if (argc != 2)
    {
        printf("%s <number_processes>\n", argv[0]);
        return 0;
    }

    int maxIterations = 1000;
    double error = 0.0000000001;
    int np = atoi(argv[1]);
    double** matrix = createMatrix();
    double* constants = createArray();

    JACOBI result = jacobi(matrix, constants, maxIterations, error, np);

    if (result.converged)
    {
        printf("Interations: %d\n", result.iterations);
        // for (int i = 0; i < SIZE; i++)
        // {
        //     printf("X[%d] = %.2lf\n", i + 1, result.values[i]);
        // }
    }
    else
    {
        printf("Not converged.");
    }
}