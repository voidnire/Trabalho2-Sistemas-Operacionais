//gcc -std=c11 -Wall -Wextra -pedantic -O2 produto_sequencial.c -o prodseq
// ./prodseq 10000
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>


// Função que realiza o cálculo sequencial do produto escalar
double calcularProdutoEscalarSequencial(double vetorA[], double vetorB[], int tamanho) {
    double produto_escalar = 0.0;
    for (int i = 0; i < tamanho; i++) {
        produto_escalar += vetorA[i] * vetorB[i];
    }
    return produto_escalar;
}


int main(int argc, char *argv[]) {
    int tam_vetor;
    double *vetor1, *vetor2;
    double ts;
    clock_t start, end;

    long cpus = sysconf(_SC_NPROCESSORS_ONLN);
    if (cpus < 1) cpus = 1;

    if (argc < 2) {
        fprintf(stderr, "Uso: %s <tamanho_do_vetor>\n", argv[0]);
        return 1;
    }

    long val_n = atol(argv[1]);
    if (val_n <= 0) {
        fprintf(stderr, "Tamanho do vetor inválido: %lds\n", val_n);
        return 1;
    }

    tam_vetor = (int) val_n;


    
   
    
    // Alocação de Memória
    vetor1 = (double *)malloc(tam_vetor * sizeof(double));
    vetor2 = (double *)malloc(tam_vetor * sizeof(double));

    if (vetor1 == NULL || vetor2 == NULL) {
        perror("Erro de alocação de memória.");
        free(vetor1); free(vetor2);
        return 1;
    }

    // Inicialização dos Vetores (Valores aleatórios para simulação)
    srand(time(NULL)); // Inicializa o gerador de números aleatórios
    for (int i = 0; i < tam_vetor; i++) {
        vetor1[i] = (double)rand() / RAND_MAX;
        vetor2[i] = (double)rand() / RAND_MAX;
        
    }
     sleep(5); //para deixar fora a geração

    // CÁLCULO SEQUENCIAL E MEDIÇÃO DO TEMPO (Ts)
    start = clock();
    double resultado_sequencial = calcularProdutoEscalarSequencial(vetor1, vetor2, tam_vetor);
    end = clock();
    ts = ((double) (end - start)) / CLOCKS_PER_SEC;

    

    /*printf("\n--- Resultado ---\n");
    printf("Tamanho do Vetor: %d\n", tam_vetor);
    printf("Resultado Sequencial: %.6f\n", resultado_sequencial);
    printf("Tempo Sequencial (Ts): %.6f segundos\n", ts);
    printf("CPUS: %ld\n",cpus);*/

    printf("\nCSV_DATA;");
    printf("computador: linux_erin;");
    printf(" tam_vetor: %d;", tam_vetor);
    printf(" n_threads: 1;"); // nao tem multithreading
    printf(" n_cpus: %ld;", cpus);
    printf(" resultado: %.12f;", resultado_sequencial);
    printf(" tp: 0.0;"); ///tempo total paralelo em s
    printf(" ts: %.6f;",ts); ///tempo total sequencial em s
    printf("\n");
    

    // Liberação de Memória
    free(vetor1);
    free(vetor2);

    return 0;
}