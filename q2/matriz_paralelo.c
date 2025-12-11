// gcc -std=c11 -Wall -Wextra -pedantic -O3 -pthread matriz_paralelo.c -o matpar
// ./matpar 1000 4
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <string.h>

// Estrutura de argumentos para as threads
typedef struct {
    double *A;
    double *B;
    double *C;
    int N;
    int start_row; // Linha inicial que a thread vai calcular
    int end_row;   // Linha final (exclusiva)
} thread_arg_t;

/* Função Worker (Faz o trabalho pesado)
void *multiplicarMatrizParalelo(void *arg) {
    thread_arg_t *ta = (thread_arg_t *) arg;
    int N = ta->N;

    // Loop apenas nas linhas designadas para esta thread
    for (int i = ta->start_row; i < ta->end_row; i++) {
        for (int j = 0; j < N; j++) {
            double soma = 0.0;
            for (int k = 0; k < N; k++) {
                soma += ta->A[i * N + k] * ta->B[k * N + j];
            }
            ta->C[i * N + j] = soma;
        }
    }
    return NULL;
}*/

// Função Worker OTIMIZADA (Usa a matriz B já transposta)
void *multiplicarMatrizParalelo(void *arg) {
    thread_arg_t *ta = (thread_arg_t *) arg;
    int N = ta->N;
    double *A = ta->A;
    double *B_T = ta->B; // O ponteiro recebido JÁ É A TRANSPOSTA
    double *C = ta->C;

    // Loop apenas nas linhas designadas para esta thread
    for (int i = ta->start_row; i < ta->end_row; i++) {
        for (int j = 0; j < N; j++) {
            double soma = 0.0;
            for (int k = 0; k < N; k++) {
                // OTIMIZAÇÃO AQUI:
                // A[i][k] * B_T[j][k] (Acesso linear em ambas!)
                // Note que acessamos B_T usando [j * N + k]
                soma += A[i * N + k] * B_T[j * N + k];
            }
            C[i * N + j] = soma;
        }
    }
    return NULL;
}

static double timespec_diff_seconds(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {
    int N = 0, num_threads = 0;
    double *A = NULL, *B = NULL, *C = NULL;
    long cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpus < 1) cpus = 1;

    if (argc < 3) {
        fprintf(stderr, "Uso: %s <tamanho_matriz> <num_threads>\n", argv[0]);
        return 1;
    }

    N = atoi(argv[1]);
    num_threads = atoi(argv[2]);

    if (N <= 0 || num_threads <= 0) {
        fprintf(stderr, "Parametros invalidos.\n");
        return 1;
    }

    // Se tiver mais threads que linhas, limita as threads
    if (num_threads > N) num_threads = N;

    // Alocação linear (Matriz Flattened)
    A = malloc(N * N * sizeof(double));
    B = malloc(N * N * sizeof(double));
    C = malloc(N * N * sizeof(double));

    if (!A || !B || !C) {
        perror("malloc");
        free(A); free(B); free(C);
        return 1;
    }

    srand(42); // Mesma semente do sequencial para comparação justa
    for (int i = 0; i < N * N; ++i) {
        A[i] = (double)rand() / RAND_MAX;
        B[i] = (double)rand() / RAND_MAX;
    }

    // Aloca matriz para a transposta de B
    // colunas de B em linhas de B_T para acesso rápido na thread.
    double *B_T = (double *)malloc(N * N * sizeof(double));
    if (!B_T) {
        perror("Erro de alocação B_T");
        free(A); free(B); free(C);
        return 1;
    }

    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            B_T[j * N + i] = B[i * N + j];
        }
    }

    // --- Versão Paralela (Tp) ---
    struct timespec t0, t1;
    pthread_t *threads = malloc(num_threads * sizeof(pthread_t));
    thread_arg_t *args = malloc(num_threads * sizeof(thread_arg_t));

    // Divisão de Carga (Load Balancing) igual ao código do seu amigo
    int base = N / num_threads;
    int resto = N % num_threads;
    int offset = 0;

    clock_gettime(CLOCK_MONOTONIC, &t0);

    for (int t = 0; t < num_threads; ++t) {
        int rows = base + (t < resto ? 1 : 0); // Distribui o resto
        
        args[t].A = A;
        args[t].B = B_T; // TRANSPOSTA
        args[t].C = C;
        args[t].N = N;
        args[t].start_row = offset;
        args[t].end_row = offset + rows;

        pthread_create(&threads[t], NULL, multiplicarMatrizParalelo, &args[t]);
        
        offset += rows;
    }

    for (int t = 0; t < num_threads; ++t) {
        pthread_join(threads[t], NULL);
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double tp = timespec_diff_seconds(t0, t1);

    // Checksum para validação
    double check_sum = 0.0;
    for(int i = 0; i < N*N; i+=N) check_sum += C[i];

    printf("\nCSV_DATA;");
    printf("computador: linux_erin_6;");
    printf(" tam_matriz: %d;", N);
    printf(" n_threads: %d;", num_threads);
    printf(" n_cpus: %ld;", cpus);
    printf(" checksum: %.2f;", check_sum);
    printf(" tp: %.6f;", tp);
    printf(" ts: 0.0;");
    printf("\n");

    free(A); free(B); free(C); free(B_T);
    free(threads); free(args);
    return 0;
}