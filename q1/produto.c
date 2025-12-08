// gcc produto.c -o prod
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
typedef struct __myarg_t {
	int tam1;
	int tam2;
    double *vetor1;
    double *vetor2;
    double produto_escalar;
} myarg_t;

// ------------ TAMANHOS DIFERENTES --------------
// Função auxiliar para encontrar o menor entre dois números
int min(int a, int b) {
    return (a < b) ? a : b;
}

double calcularProdutoEscalar( //suporta Diferentes Tamanhos
    double vetorA[], int tamanhoA, 
    double vetorB[],int tamanhoB
) {
    // 1. Encontra o tamanho limite (o menor entre os dois)
    int tamanho_limite = min(tamanhoA, tamanhoB);
    double produto_escalar = 0.0;
    
    // 2. Verifica se algum vetor é vazio
    if (tamanho_limite <= 0) {
        return 0.0; // Produto escalar de vetores vazios ou inválidos é zero
    }
    
    // 3. Loop apenas até o tamanho_limite
    for (int i = 0; i < tamanho_limite; i++) {
        produto_escalar += vetorA[i] * vetorB[i];
    }
    
    return produto_escalar;
}

void *mythread(void *arg) { // precisa receber void* e retornar void*
    myarg_t *m = (myarg_t *) arg;
    
     // 1. Encontra o tamanho limite (o menor entre os dois)
    int tamanho_limite = min(m->tam1, m->tam2);
    int i;
    
    double soma =0.0;
    
    // 2. Verifica se algum vetor é vazio
    if (tamanho_limite <= 0) {
        return NULL; // Produto escalar de vetores vazios ou inválidos é zero
    }
    
    // 3. Loop apenas até o tamanho_limite
    for (i = 0; i < tamanho_limite; i++) {
        soma += m->vetor1[i] * m->vetor2[i];
    }
    m->produto_escalar = soma; 
        
	return NULL;
}
// ------------ TAMANHOS DIFERENTES --------------

void imprimirVetor(double vetor[], int tamanho) {
    printf("(");
    for (int i = 0; i < tamanho; i++) {
        printf("%.2f%s", vetor[i], (i == tamanho - 1) ? "" : ", ");
    }
    printf(")");
}

int main() {
    // Definindo e inicializando os vetores
    myarg_t args;
    double resultado;
    printf("--------------------THREADS PARALELAS--------------------\n");
    printf("Informe o tamanho do PRIMEIRO vetor:\n");
    scanf("%d",&args.tam1);
    
    /// ALOCANDO MEMÓRIA PRO PRIMEIRO VETOR
    args.vetor1 = (double *)malloc(args.tam1 * sizeof(double));
    if (args.vetor1 == NULL) {
        printf("Erro de alocação de memória!\n");
        return 1;
    }
    /// ALOCANDO MEMÓRIA PRO PRIMEIRO VETOR

    printf("\nInforme os %d números do PRIMEIRO vetor:\n", args.tam1);
    for (int i = 0; i < args.tam1; i++) {
        printf("Vetor1[%d]: ", i);
        scanf("%lf", &args.vetor1[i]);
    }

    printf("------------------------------------");

    printf("\nInforme o tamanho do SEGUNDO vetor: \n");
    scanf("%d",&args.tam2);
 


    /// ALOCANDO MEMÓRIA PRO SEGUNDO VETOR
    args.vetor2 = (double *)malloc(args.tam2 * sizeof(double));
    if (args.vetor2 == NULL) {
        printf("Erro de alocação de memória!\n");
        free(args.vetor1); // Liberar o primeiro vetor também
        return 1;
    }
    /// ALOCANDO MEMÓRIA PRO SEGUNDO VETOR

    printf("\nInforme os %d números do SEGUNDO vetor:\n", args.tam2);
    for (int i = 0; i < args.tam2; i++) {
        printf("Vetor2[%d]: ", i);
        scanf("%lf", &args.vetor2[i]);
    }

    /*int threadNumero = 0;
    printf("\n-----> THREADS <-----\n");
    printf("Quantas threads você deseja criar para rodar o algoritmo?\n");
    printf("Número de threads:  ");
    scanf("%d", &threadNumero); 

    pthread_t p[threadNumero];
    int rc;*/


    

    printf("------------------------------------");

    // Imprimindo o resultado
    printf("\n--- Resultado do Produto Escalar ---\n");
   
    /*resultado = calcularProdutoEscalar(
        args.vetor1, args.tam1, 
        args.vetor2, args.tam2
    );*/
    pthread_t p;
    int rc = pthread_create(&p, NULL, mythread, &args); 
    if (rc != 0) {
        fprintf(stderr, "Erro ao criar a thread: %d\n", rc);
        // Liberação de memória em caso de falha na criação da thread
        free(args.vetor1);
        free(args.vetor2);
        return 1;
    }

    pthread_join(p, NULL);// main espera a thread p terminar o cálculo

    printf("O Produto Escalar é: **%.2lf**\n", args.produto_escalar);

    // Liberar a memória alocada
    free(args.vetor1);
    free(args.vetor2);

    
    return 0;
}

