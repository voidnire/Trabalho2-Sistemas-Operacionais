
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>
#include <pthread.h>
#include <string.h> // Para memset

//Programa paralelo — divide o trabalho por T threads; 
//cada thread calcula uma soma parcial em seu bloco; 
//no final soma-se as parciais.


// Define o tamanho da linha de cache, tipicamente 64 bytes
#define CACHELINE_SIZE 64
// Calcula quantos doubles cabem em 64 bytes (64 / 8) - 1 (para o valor)
#define PADDING_DOUBLES (CACHELINE_SIZE / sizeof(double) - 1)

// Estrutura para evitar False Sharing
// Garante que cada soma parcial esteja em sua própria linha de cache.
typedef struct {
    double value;
    double padding[PADDING_DOUBLES];
} padded_double;


typedef struct {
    const double *a; //vetores de entrada
    const double *b;
    size_t n; // tamanho total do vetor
    int tid; // id da thread
    int nthreads; //total de threads
    padded_double *partials; //ponteiro para o array onde 
    //cada thread escreverá sua soma parcial
} thread_arg_t; 


double* gen_vector(size_t n, unsigned long seed) { // tamanho e seed p/ random
    double *v = malloc(n * sizeof(double));
    if (!v) { perror("malloc"); exit(1); }
    srand((unsigned)seed); 
    for (size_t i = 0; i < n; ++i) {
        v[i] = 2.0 * (double)rand() / (double)RAND_MAX - 1.0;
    }
    return v;
} // Gera vetores double com valores em [-1,1].


/**
 * @brief Versão PARALELA: Função executada por cada thread.
 */

void *mythread(void *arg){
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
    // Usa t->tid como índice para armazenar no slot correto da thread
    t->partials[t->tid].value = sum;
    
    printf("Thread %d calculou a soma parcial %.4f (índices %zu a %zu)\n", 
           t->tid, sum, start, end - 1);
    
    return NULL;
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

int main(int argc, char *argv[])
{
    const size_t N = 100; // Tamanho do vetor (10 milhões)
    int T;           // Número de threads
    double final_result = 0.0;
    double result_s = 0.0;

    // Variáveis de tempo
    struct timespec start, end;
    double Ts, Tp, Sp;
    
    // NÚMERO DE threads
    if (argc < 2) {
        // Se o usuário não fornecer argumentos, usa um valor padrão (ex: 4)
        T = 4;
        printf("Nenhum argumento fornecido. Usando valor padrao de T = %d threads.\n", T);
    } else {
        // Converte o primeiro argumento (argv[1]) de string para int
        T = atoi(argv[1]);
        if (T <= 0) {
            fprintf(stderr, "ERRO: O numero de threads deve ser um inteiro positivo.\n");
            return 1;
        }
    }
    // NÚMERO DE threads

    printf("Tamanho do vetor (N): %zu\n", N);
    printf("Numero de threads (T): %d\n\n", T);

    
    printf("Gerando vetores de tamanho N=%zu...\n", N);
    double *a = gen_vector(N, 12345); // Seed 12345
    double *b = gen_vector(N, 54321); // Seed 54321
    
    // 2. Alocação de memória para threads e somas parciais
    pthread_t *threads = malloc(T * sizeof(pthread_t));
    thread_arg_t *args = malloc(T * sizeof(thread_arg_t));
    padded_double *partials = malloc(T * sizeof(padded_double));
    
    if (!threads || !args || !partials) { perror("malloc"); exit(1); }

    // Zera o array de somas parciais por segurança
    memset(partials, 0, T * sizeof(padded_double));

    // 3. Criação das threads
    printf("Criando %d threads...\n", T);
    for (int i = 0; i < T; ++i) {
        // Inicializa os argumentos para a thread i
        args[i].a = a;
        args[i].b = b;
        args[i].n = N;
        args[i].tid = i;           // ID da thread: 0, 1, 2, ...
        args[i].nthreads = T;
        args[i].partials = partials; // Ponteiro para o array de somas parciais
        
        // Cria a thread i
        int rc = pthread_create(&threads[i], NULL, mythread, &args[i]);
        if (rc) {
            fprintf(stderr, "ERRO: pthread_create falhou (código %d)\n", rc);
            exit(1);
        }
    }

    // 4. Espera pelas threads
    printf("Aguardando threads finalizarem...\n");
    for (int i = 0; i < T; ++i) {
        pthread_join(threads[i], NULL);
    }
    
    // 5. Soma os resultados parciais
    printf("Somando os resultados parciais:\n");
    for (int i = 0; i < T; ++i) {
        final_result += partials[i].value;
    }

    // 6. Imprime o resultado final e limpa a memória
    printf("----------------------------------------\n");
    printf("Produto Escalar Final (Paralelo): %.6f\n", final_result);
    printf("----------------------------------------\n");

    free(a);
    free(b);
    free(threads);
    free(args);
    free(partials);
    
    return 0;
}