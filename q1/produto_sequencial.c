    // gcc produto_sequencial.c -o prod
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

#define MAX_SIZE 50 

// Função que realiza o cálculo sequencial do produto escalar
double calcularProdutoEscalarSequencial(double vetorA[], double vetorB[], int tamanho) {
    double produto_escalar = 0.0;
    for (int i = 0; i < tamanho; i++) {
        produto_escalar += vetorA[i] * vetorB[i];
    }
    return produto_escalar;
}

long imprimirNumeroCPUs() {
    // _SC_NPROCESSORS_ONLN retorna o número de unidades de processamento (núcleos lógicos) em operação.
    long num_cpus = sysconf(_SC_NPROCESSORS_ONLN);
    
    if (num_cpus > 0) {
        printf("\n[INFO] Número de CPUs/Núcleos Lógicos disponíveis: %ld\n\n", num_cpus);
    } else {
        printf("\n[INFO] Não foi possível determinar o número de CPUs. Assumindo 1.\n\n");
        num_cpus = 1; 
    }
    
    return num_cpus;
}

int main() {
    int tam_vetor;
    double *vetor1, *vetor2;
    double ts;
    clock_t start, end;

    long cpus_disponiveis = imprimirNumeroCPUs();

    printf("--- Produto Escalar Sequencial ---\n");
    
    // Leitura do Tamanho do Vetor
    printf("Informe o tamanho dos vetores: ");
    if (scanf("%d", &tam_vetor) != 1 || tam_vetor <= 0) {
        printf("Tamanho inválido. Usando %d como padrão.\n", MAX_SIZE);
        tam_vetor = MAX_SIZE;
    }
    
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

    printf("\n--- Resultado ---\n");
    printf("Tamanho do Vetor: %d\n", tam_vetor);
    printf("Resultado Sequencial: %.6f\n", resultado_sequencial);
    printf("Tempo Sequencial (Ts): %.6f segundos\n", ts);

    // Liberação de Memória
    free(vetor1);
    free(vetor2);

    return 0;
}