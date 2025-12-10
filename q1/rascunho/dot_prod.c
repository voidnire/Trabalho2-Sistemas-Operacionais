#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>    // <-- INCLUSÃO NECESSÁRIA PARA MEDIR O TEMPO
#include <pthread.h>
#include <string.h>  // Para memset

// Define o tamanho da linha de cache
#define CACHELINE_SIZE 64
#define PADDING_DOUBLES (CACHELINE_SIZE / sizeof(double) - 1)

// Estrutura para evitar False Sharing
typedef struct {
    double value;
    double padding[PADDING_DOUBLES];
} padded_double;

// Estrutura para os argumentos da thread
typedef struct {
    const double *a; 
    const double *b;
    size_t n;        
    int tid;         
    int nthreads;    
    padded_double *partials; 
} thread_arg_t; 

/**
 * @brief Gera um vetor de doubles com valores em [-1, 1].
 */
double* gen_vector(size_t n, unsigned long seed) { 
    double *v = malloc(n * sizeof(double));
    if (!v) { perror("malloc"); exit(1); }
    srand((unsigned)seed); 
    for (size_t i = 0; i < n; ++i) {
        v[i] = 2.0 * (double)rand() / (double)RAND_MAX - 1.0;
    }
    return v;
}

/**
 * @brief Versão SEQUENCIAL: Calcula o produto escalar em um único loop. (Ts)
 */
double dot_product_sequential(const double *a, const double *b, size_t n) {
    double sum = 0.0;
    for (size_t i = 0; i < n; ++i) {
        sum += a[i] * b[i];
    }
    return sum;
}

/**
 * @brief Versão PARALELA: Função executada por cada thread.
 */
void *mythread(void *arg) {
    thread_arg_t *t = (thread_arg_t *) arg;
    double sum = 0.0;
    
    // Calcula o bloco de trabalho para a thread atual
    size_t chunk_size = t->n / t->nthreads;
    size_t start = t->tid * chunk_size;
    size_t end = (t->tid == t->nthreads - 1) ? t->n : (t->tid + 1) * chunk_size;

    // Calcula a soma parcial (produto escalar)
    for (size_t i = start; i < end; ++i) {
        sum += t->a[i] * t->b[i];
    }
    
    // Armazena a soma parcial no array compartilhado
    t->partials[t->tid].value = sum;
    
    // Removi o printf aqui para nao poluir a saida em N grande
    
    return NULL;
}

int main(int argc, char *argv[]) {
    // Definições de tamanho e threads
    const size_t N = 10000000; // Tamanho do vetor (10 milhões para teste de desempenho)
    int T;                     // Número de threads
    double final_result_p = 0.0;
    double result_s = 0.0;

    // Variáveis de tempo
    struct timespec start, end;
    double Ts, Tp, Sp;

    // 1. OBTENÇÃO DO NÚMERO DE THREADS (T)
    if (argc < 2) {
        T = 4;
        printf("Nenhum argumento fornecido. Usando T = %d threads.\n", T);
    } else {
        T = atoi(argv[1]);
        if (T <= 0) {
            fprintf(stderr, "ERRO: O numero de threads deve ser um inteiro positivo.\n");
            return 1;
        }
    }
    printf("Tamanho do vetor (N): %zu\n", N);
    printf("Numero de threads (T): %d\n\n", T);

    // 2. Geração dos vetores A e B
    double *a = gen_vector(N, 12345); // Seed 12345
    double *b = gen_vector(N, 54321); // Seed 54321

    // ----------------------------------------------------------------------
    // MEDIÇÃO SEQUENCIAL (Ts)
    // ----------------------------------------------------------------------
    printf("-> Executando Versao SEQUENCIAL...\n");
    clock_gettime(CLOCK_MONOTONIC, &start); // Inicia cronômetro

    result_s = dot_product_sequential(a, b, N);

    clock_gettime(CLOCK_MONOTONIC, &end); // Para cronômetro
    
    // Calcula a diferença de tempo em segundos
    Ts = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;
    
    printf("Resultado Sequencial: %.6f\n", result_s);
    printf("Tempo Sequencial (Ts): %.6f segundos\n", Ts);
    printf("----------------------------------------\n");

    // ----------------------------------------------------------------------
    // MEDIÇÃO PARALELA (Tp)
    // ----------------------------------------------------------------------
    printf("-> Executando Versao PARALELA (%d threads)...\n", T);
    
    // Alocação de memória (AGORA USANDO O VALOR LIDO DE T)
    pthread_t *threads = malloc(T * sizeof(pthread_t));
    thread_arg_t *args = malloc(T * sizeof(thread_arg_t));
    padded_double *partials = malloc(T * sizeof(padded_double));
    
    if (!threads || !args || !partials) { perror("malloc"); exit(1); }
    memset(partials, 0, T * sizeof(padded_double));

    clock_gettime(CLOCK_MONOTONIC, &start); // Inicia cronômetro
    
    // Criação das threads
    for (int i = 0; i < T; ++i) {
        args[i].a = a;
        args[i].b = b;
        args[i].n = N;
        args[i].tid = i;
        args[i].nthreads = T;
        args[i].partials = partials;
        
        int rc = pthread_create(&threads[i], NULL, mythread, &args[i]);
        if (rc) { fprintf(stderr, "ERRO: pthread_create falhou\n"); exit(1); }
    }

    // Espera pelas threads
    for (int i = 0; i < T; ++i) {
        pthread_join(threads[i], NULL);
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end); // Para cronômetro
    
    // Soma os resultados parciais
    for (int i = 0; i < T; ++i) {
        final_result_p += partials[i].value;
    }
    
    // Calcula o tempo paralelo
    Tp = (end.tv_sec - start.tv_sec) + (end.tv_nsec - start.tv_nsec) / 1000000000.0;

    printf("Resultado Paralelo:   %.6f\n", final_result_p);
    printf("Tempo Paralelo (Tp):  %.6f segundos\n", Tp);
    printf("----------------------------------------\n");


    // ----------------------------------------------------------------------
    // CÁLCULO DA ACELERAÇÃO (Speedup - Sp)
    // ----------------------------------------------------------------------
    Sp = Ts / Tp;
    printf("\nMETRICA DE DESEMPENHO:\n");
    printf("Aceleracao (Speedup - Sp): Ts / Tp = %.2f\n", Sp);
    printf("----------------------------------------\n");
    
    // 7. Limpa a memória
    free(a);
    free(b);
    free(threads);
    free(args);
    free(partials);
    
    return 0;
}
 //flag -lrt (para o clock_gettime
// gcc -o produto_escalar seu_arquivo.c -pthread -lrt

//./produto_escalar 8