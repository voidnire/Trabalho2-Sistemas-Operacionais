// gcc -std=c11 -Wall -Wextra -pedantic -O3 matriz_sequencial.c -o matseq
// ./matseq 1000
#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

// função pra medir tempo 
static double timespec_diff_seconds(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {
    int N;
    double *A, *B, *C;
    long cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpus < 1) cpus = 1;

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <tamanho_da_matriz_N>\n", argv[0]);
        return 1;
    }

    long val_n = atol(argv[1]);
    if (val_n <= 0) {
        fprintf(stderr, "Tamanho inválido: %ld\n", val_n);
        return 1;
    }
    N = (int) val_n;

    // Alocação como vetor linear (N*N) para evitar fragmentação de memória
    
    A = (double *)malloc(N * N * sizeof(double));
    B = (double *)malloc(N * N * sizeof(double));
    C = (double *)malloc(N * N * sizeof(double));

    if (!A || !B || !C) {
        perror("Erro de alocação");
        free(A); free(B); free(C);
        return 1;
    }


    
    // Inicialização (Semente fixa 42 para consistência)
    srand(42); 
    for (int i = 0; i < N * N; i++) {
        A[i] = (double)rand() / RAND_MAX;
        B[i] = (double)rand() / RAND_MAX;
    }

    // Aloca matriz para a transposta de B
    // acessa endereços de memória contíguos
    double *B_T = (double *)malloc(N * N * sizeof(double));
    if (!B_T) {
        perror("Erro de alocação B_T");
        free(A); free(B); free(C);
        return 1;
    }

    // Medição de Tempo
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    // Multiplicação Clássica O(N^3)
    // C[i][j] += A[i][k] * B[k][j]
    /* Mapeamento linear: Mat[i][j] = Mat[i * N + j]
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double soma = 0.0;
            for (int k = 0; k < N; k++) {
                soma += A[i * N + k] * B[k * N + j];
            }
            C[i * N + j] = soma;
        }
    }*/

    // 1. TRANSPOSIÇÃO DE B (O(N^2))
    // Transforma colunas de B em linhas de B_T
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            // B_T[linha j][coluna i] = B[linha i][coluna j]
            B_T[j * N + i] = B[i * N + j];
        }
    }

    // 2. MULTIPLICAÇÃO OTIMIZADA (O(N^3))
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            double soma = 0.0;
            // Agora 'k' percorre as colunas de A e as LINHAS de B_T
            // Ambos os acessos são sequenciais na memória (Acesso rápido!)
            for (int k = 0; k < N; k++) {
                // ANTES:  A[i * N + k] * B[k * N + j]  <- Pulo de memória em B
                // AGORA:  A[i * N + k] * B_T[j * N + k] <- Acesso contíguo em B_T
                soma += A[i * N + k] * B_T[j * N + k];
            }
            C[i * N + j] = soma;
        }
    }

    

    clock_gettime(CLOCK_MONOTONIC, &end);
    double ts = timespec_diff_seconds(start, end);

    // Soma de verificação simples (para colocar no CSV no lugar do resultado)
    double check_sum = 0.0;
    for(int i = 0; i < N*N; i+=N) check_sum += C[i]; // Soma apenas diagonal/amostra pra ser rápido

    printf("\nCSV_DATA;");
    printf("computador: gitspace_erin_2;");
    printf(" tam_matriz: %d;", N);
    printf(" n_threads: 1;");
    printf(" n_cpus: %ld;", cpus);
    printf(" checksum: %.2f;", check_sum); // Checksum em vez de resultado inteiro
    printf(" tp: 0.0;");
    printf(" ts: %.6f;", ts);
    printf("\n");

    free(A); free(B); free(C); free(B_T);
    return 0;
}