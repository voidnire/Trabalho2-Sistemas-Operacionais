#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <time.h>
#include <termios.h>
#include <ctype.h>

// --- ESTADO DO SATÉLITE ---
int bateria = 100;
int temperatura = 25;
char status_sistema[100] = "EM ORBITA (STANDBY)";
int simulacao_rodando = 1;

// Buffer Global de Input
char input_buffer[256] = {0};
int input_pos = 0;

pthread_mutex_t mutex_estado = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_io = PTHREAD_MUTEX_INITIALIZER; 

struct termios orig_termios;

void disableRawMode() {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

void enableRawMode() {
    tcgetattr(STDIN_FILENO, &orig_termios);
    atexit(disableRawMode);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

// --- FUNÇÃO DE PRINT SEGURO (PARA THREADS DE FUNDO) ---
void print_log_seguro(char* msg, long thread_id, int tipo_msg) {
    pthread_mutex_lock(&mutex_io);

    printf("\r\033[K"); 

    // Cores
    if (tipo_msg == 1) printf("\033[1;31m");      // Vermelho
    else if (tipo_msg == 2) printf("\033[1;33m"); // Amarelo
    else if (tipo_msg == 3) printf("\033[1;34m"); // Azul
    else if (tipo_msg == 4) printf("\033[1;35m"); // Roxo
    else printf("\033[1;30m");                    // Cinza

    if (thread_id > 0)
        printf("[THREAD %ld] %s\033[0m\n", thread_id, msg);
    else if (tipo_msg == 4)
        printf("[RADIO] %s\033[0m\n", msg);
    else
        printf("[SISTEMA] %s\033[0m\n", msg);

   
    printf("COMANDO > %s", input_buffer);
    fflush(stdout);

    pthread_mutex_unlock(&mutex_io);
}

long get_thread_id() {
    return syscall(SYS_gettid) % 1000;
}

//  THREAD DE TELEMETRIA 
void* thread_telemetria(void* arg) {
    long id = get_thread_id();
    char msg[100];
    
    while (simulacao_rodando) {
        pthread_mutex_lock(&mutex_estado);

        if (bateria > 0 && (rand() % 5 == 0)) bateria--;
        int em_manobra = (strstr(status_sistema, "MANOBRA") != NULL);
        
        if (em_manobra) {
            if (rand() % 2 == 0) temperatura += 2; 
        } else {
            if (temperatura > 25) temperatura -= 2;
        }

        // Monitoramento
        if (em_manobra) {
            sprintf(msg, "Monitorando propulsão... Temp:%dC", temperatura);
            print_log_seguro(msg, id, 0); 
        }
        else if (temperatura > 80) {
            sprintf(msg, "ALERTA: SUPERAQUECIMENTO (%d C)!", temperatura);
            print_log_seguro(msg, id, 1);
        } 
        else if (bateria < 10) {
            sprintf(msg, "ALERTA: BATERIA CRITICA (%d%%)!", bateria);
            print_log_seguro(msg, id, 2);
        }

        pthread_mutex_unlock(&mutex_estado);
        usleep(3000000 + (rand() % 2000000)); 
    }
    return NULL;
}

//  THREAD DE MANOBRA 
void* thread_manobra(void* arg) {
    int duracao = *((int*)arg);
    free(arg); 
    char msg[100];

    pthread_mutex_lock(&mutex_estado);
    sprintf(status_sistema, "EXECUTANDO MANOBRA DE ORBITA (%ds)...", duracao);
    sprintf(msg, "Propulsores principais ativados. Duração: %ds", duracao);
    print_log_seguro(msg, 0, 3); 
    pthread_mutex_unlock(&mutex_estado);

    sleep(duracao);

    pthread_mutex_lock(&mutex_estado);
    strcpy(status_sistema, "PROPULSORES DESLIGADOS. ORBITA ESTAVEL.");
    sprintf(msg, "Manobra de órbita finalizada com sucesso.");
    print_log_seguro(msg, 0, 3);
    pthread_mutex_unlock(&mutex_estado);

    return NULL;
}

//  THREAD DOWNLINK 
void* thread_downlink(void* arg) {
    char msg[100];
    
    pthread_mutex_lock(&mutex_estado);
    sprintf(msg, "Iniciando transmissão de dados para Houston...");
    print_log_seguro(msg, 0, 4); 
    pthread_mutex_unlock(&mutex_estado);

    for(int i = 20; i <= 100; i += 20) {
        sleep(2); 
        pthread_mutex_lock(&mutex_estado);
        if(bateria > 0) bateria--; 
        sprintf(msg, "Enviando pacotes de telemetria... [%d%%]", i);
        print_log_seguro(msg, 0, 4);
        pthread_mutex_unlock(&mutex_estado);
    }

    sprintf(msg, "Transmissão de dados concluída.");
    print_log_seguro(msg, 0, 4);
    return NULL;
}

void imprimir_interface() {
    pthread_mutex_lock(&mutex_estado); 
    pthread_mutex_lock(&mutex_io);     
    
    printf("\r\033[K");
    printf("-------------------------------------------------\n");
    printf("   AGC - SATELITE MULTITASK                      \n");
    printf("-------------------------------------------------\n");
    printf(" BATERIA: %d%%  |  TEMP: %d C  |  STATUS: %s\n", bateria, temperatura, status_sistema);
    printf("-------------------------------------------------\n");
    printf("COMANDOS: [ORBITA] [BAIXAR] [CARREGAR] [STATUS] [SAIR]\n");
    
    pthread_mutex_unlock(&mutex_io);     
    pthread_mutex_unlock(&mutex_estado); 
}

int main() {
    srand(time(NULL));
    int qtd_threads;

    printf("--- CONFIGURACAO DE SISTEMA ---\n");
    printf("Quantas THREADS de monitoramento? ");
    scanf("%d", &qtd_threads);
    int trash; while ((trash = getchar()) != '\n' && trash != EOF);
    if (qtd_threads < 1) qtd_threads = 1;

    enableRawMode(); 

    printf("Iniciando %d thread(s)...\n", qtd_threads);
    pthread_t* threads = malloc(sizeof(pthread_t) * qtd_threads);
    for(int i = 0; i < qtd_threads; i++) {
        pthread_create(&threads[i], NULL, thread_telemetria, NULL);
    }

    imprimir_interface();
    pthread_mutex_lock(&mutex_io);
    printf("COMANDO > ");
    fflush(stdout);
    pthread_mutex_unlock(&mutex_io);

    char c;
    while (simulacao_rodando) {
        if (read(STDIN_FILENO, &c, 1) == 1) {
            
            pthread_mutex_lock(&mutex_io); 

            if (c == '\n') { // ENTER
                printf("\n"); 
                input_buffer[input_pos] = '\0'; 
                
                pthread_mutex_unlock(&mutex_io);

                // PROCESSAMENTO DE COMANDO
                if (strlen(input_buffer) > 0) {
                    
                    if (strcmp(input_buffer, "ORBITA") == 0) {
                        int* duracao = malloc(sizeof(int));
                        *duracao = 30; 
                        pthread_t t_manobra;
                        pthread_create(&t_manobra, NULL, thread_manobra, duracao);
                        pthread_detach(t_manobra); 
                    }
                    else if (strcmp(input_buffer, "BAIXAR") == 0) {
                        pthread_t t_down;
                        pthread_create(&t_down, NULL, thread_downlink, NULL);
                        pthread_detach(t_down);
                    }
                    else if (strcmp(input_buffer, "CARREGAR") == 0) {
                        pthread_mutex_lock(&mutex_estado);
                        bateria = 100;
                        sprintf(status_sistema, "SISTEMA RECARREGADO.");
                        pthread_mutex_unlock(&mutex_estado);

                        pthread_mutex_lock(&mutex_io);
                        printf("\r\033[K\033[1;34m[SISTEMA] Baterias recarregadas.\033[0m\n");
                        pthread_mutex_unlock(&mutex_io);
                    }
                    else if (strcmp(input_buffer, "STATUS") == 0) {
                        imprimir_interface(); 
                    }
                    else if (strcmp(input_buffer, "SAIR") == 0) {
                        simulacao_rodando = 0;
                    }
                    else {
                      
                        pthread_mutex_lock(&mutex_io);
                        printf("\r\033[K\033[1;31m[ERRO] Comando desconhecido.\033[0m\n");
                        pthread_mutex_unlock(&mutex_io);
                    }
                }

                pthread_mutex_lock(&mutex_io);
                memset(input_buffer, 0, sizeof(input_buffer));
                input_pos = 0;
                if (simulacao_rodando) printf("COMANDO > ");
            
            } else if (c == 127 || c == '\b') { // BACKSPACE
                if (input_pos > 0) {
                    input_pos--;
                    input_buffer[input_pos] = '\0';
                    printf("\b \b"); 
                }
            } else if (!iscntrl(c) && input_pos < 255) { 
                input_buffer[input_pos++] = c;
                input_buffer[input_pos] = '\0';
                printf("%c", c); 
            }
            
            fflush(stdout);
            pthread_mutex_unlock(&mutex_io); 
        }
        usleep(1000); 
    }

    free(threads);
    pthread_mutex_destroy(&mutex_estado);
    pthread_mutex_destroy(&mutex_io);
}
