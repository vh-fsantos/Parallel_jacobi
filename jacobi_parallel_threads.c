#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/wait.h>
#include <pthread.h>

#define SIZE 10000

//---------------------------------------------------------

struct jacobiResult{
    int iterations;
    double values[SIZE];
    int converged;
} typedef JACOBI;


/*--------------------------------------------------------------------*/

struct parameters{
	int id;
	int nthr;
	double *x;
	double *new_x;
	double **matrix;
    double *constants;
};
typedef struct parameters PARAMETERS;

//---------------------------------------------------------

double norma_vector(double *x){
    double sum = 0;
    for (int i = 0; i < SIZE; i++){
        sum += pow(x[i], 2);
    }

    return sqrt(sum);
}

//---------------------------------------------------------

void * update_x(void *args)
{
    PARAMETERS *pars = (PARAMETERS *)args;
	int id = pars->id;
	int nthr = pars->nthr;
	double *x = pars->x;
	double *new_x = pars->new_x;
	double **matrix = pars->matrix;
	double *constants = pars->constants;

    for (int i = id; i < SIZE; i += nthr)
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

JACOBI jacobi(double** matrix, double* constants, int maxIterations, double error, int nthr) 
{
    pthread_t *tid = NULL;
	PARAMETERS *pars = NULL;

    tid = (pthread_t *) malloc(nthr * sizeof(pthread_t));
	pars = (PARAMETERS *) malloc(nthr * sizeof(PARAMETERS));

    double* x; // Representa as tentativas
    double* new_x; // Vetor com o resultado final
    JACOBI result;

    x = (double *) malloc(SIZE * sizeof(double));
    new_x = (double *) malloc(SIZE * sizeof(double));
    // Inicializando o X
    for (int i = 0; i < SIZE; i++)
    {
        x[i] = 0;   
    }

    // Algoritmo do jacobi - resolver o sistema
    int k = 0;
    while (k < maxIterations) 
    {
        // Paralelizando a atualização do X
        for(int i = 0; i < nthr; i++){
            pars[i].id = i;
            pars[i].nthr = nthr;
            pars[i].x = x;
            pars[i].new_x = new_x;
            pars[i].matrix = matrix;
            pars[i].constants = constants;
            pthread_create(&tid[i], NULL, update_x, (void *) &pars[i]); 
	    } 
        // Aguardando threads
	    for (int i = 0; i < nthr; i++){
		    pthread_join(tid[i], NULL);
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

            free(x);
            free(new_x);

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
    
    free(x);
    free(new_x);

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
    if (argc != 3)
    {
        printf("%s <max_iterations> <number_threads>\n", argv[0]);
        return 0;
    }
    
    int maxIterations = atoi(argv[1]);
    double error = 0.0000000001;
    int nthr = atoi(argv[2]);
    double** matrix = createMatrix();
    double* constants = createArray();

    JACOBI result = jacobi(matrix, constants, maxIterations, error, nthr);

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

    free(matrix);
    free(constants);
}