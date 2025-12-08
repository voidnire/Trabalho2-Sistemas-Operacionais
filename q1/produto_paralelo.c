// produto_escalar_paralelo.c
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

typedef struct {
    double *vetor1;
    double *vetor2;
    int start_index;
    int end_index;
    double partial_sum;
} thread_arg_t;

void *calcularProdutoEscalarParalelo(void *arg) {
    thread_arg_t *ta = (thread_arg_t *) arg;
    double soma_local = 0.0;
    for (int i = ta->start_index; i < ta->end_index; i++) {
        soma_local += ta->vetor1[i] * ta->vetor2[i];
    }
    ta->partial_sum = soma_local;
    return NULL;
}

static double timespec_diff_seconds(struct timespec a, struct timespec b) {
    return (b.tv_sec - a.tv_sec) + (b.tv_nsec - a.tv_nsec) / 1e9;
}

int main(int argc, char *argv[]) {
    int tam_vetor = 0, num_threads = 0;
    double *vetor1 = NULL, *vetor2 = NULL;
    long cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpus < 1) cpus = 1;

    if (argc < 3) {
        fprintf(stderr, "Uso: %s <tamanho_vetor> <num_threads>\n", argv[0]);
        fprintf(stderr, "Exemplo: %s 1000000 4\n", argv[0]);
        return 1;
    }

    char *endptr = NULL;
    long val_n = strtol(argv[1], &endptr, 10);
    if (endptr == argv[1] || val_n <= 0) {
        fprintf(stderr, "Tamanho do vetor inválido: %s\n", argv[1]);
        return 1;
    }
    tam_vetor = (int) val_n;

    endptr = NULL;
    long val_p = strtol(argv[2], &endptr, 10);
    if (endptr == argv[2] || val_p <= 0) {
        fprintf(stderr, "Número de threads inválido: %s\n", argv[2]);
        return 1;
    }
    num_threads = (int) val_p;

    if (num_threads > tam_vetor) {
        num_threads = tam_vetor;
        if (num_threads < 1) num_threads = 1;
        printf("[INFO] Ajustando threads para %d (<= tam_vetor)\n", num_threads);
    }

    printf("[INFO] CPUs lógicos disponíveis: %ld\n", cpus);
    printf("[INFO] Tamanho do vetor: %d, Threads solicitadas (ajustadas): %d\n",
           tam_vetor, num_threads);

    vetor1 = malloc((size_t)tam_vetor * sizeof(double));
    vetor2 = malloc((size_t)tam_vetor * sizeof(double));
    if (!vetor1 || !vetor2) {
        perror("malloc");
        free(vetor1); free(vetor2);
        return 1;
    }

    // semente fixa para reproducibilidade (troque para time(NULL) se quiser variabilidade)
    srand(42);
    for (int i = 0; i < tam_vetor; ++i) {
        vetor1[i] = (double)rand() / RAND_MAX;
        vetor2[i] = (double)rand() / RAND_MAX;
    }

    // --- Versão sequencial (Ts) ---
    struct timespec t0, t1;
    double resultado_seq = 0.0;
    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int i = 0; i < tam_vetor; ++i) resultado_seq += vetor1[i] * vetor2[i];
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double ts = timespec_diff_seconds(t0, t1);

    // --- Versão paralela (Tp) ---
    pthread_t *threads = malloc((size_t)num_threads * sizeof(pthread_t));
    thread_arg_t *args = malloc((size_t)num_threads * sizeof(thread_arg_t));
    if (!threads || !args) {
        perror("malloc threads/args");
        free(vetor1); free(vetor2); free(threads); free(args);
        return 1;
    }

    int base = tam_vetor / num_threads;
    int resto = tam_vetor % num_threads;
    int offset = 0;

    clock_gettime(CLOCK_MONOTONIC, &t0);
    for (int t = 0; t < num_threads; ++t) {
        int end = offset + base + (t < resto ? 1 : 0);
        args[t].vetor1 = vetor1;
        args[t].vetor2 = vetor2;
        args[t].start_index = offset;
        args[t].end_index = end;
        args[t].partial_sum = 0.0;
        int rc = pthread_create(&threads[t], NULL, calcularProdutoEscalarParalelo, &args[t]);
        if (rc != 0) {
            fprintf(stderr, "pthread_create failed: %s\n", strerror(rc));
            // ajusta num_threads para aguardar só os já criados
            num_threads = t;
            break;
        }
        offset = end;
    }

    double resultado_paralelo = 0.0;
    for (int t = 0; t < num_threads; ++t) {
        int rc = pthread_join(threads[t], NULL);
        if (rc != 0) {
            fprintf(stderr, "pthread_join failed: %s\n", strerror(rc));
        } else {
            resultado_paralelo += args[t].partial_sum;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &t1);
    double tp = timespec_diff_seconds(t0, t1);

    printf("\n--- Resultados ---\n");
    printf("Tamanho do vetor: %d\n", tam_vetor);
    printf("Threads usadas: %d\n", num_threads);
    printf("Resultado sequencial: %.12f\n", resultado_seq);
    printf("Resultado paralelo:   %.12f\n", resultado_paralelo);
    printf("Tempo sequencial Ts: %.6f s\n", ts);
    printf("Tempo paralelo   Tp: %.6f s\n", tp);
    printf("Speedup Sp = Ts/Tp: %.6f\n", tp > 0.0 ? ts / tp : 0.0);
    printf("[INFO] CPUs lógicos disponíveis: %ld\n", cpus);

    free(vetor1); free(vetor2); free(threads); free(args);
    return 0;
}
