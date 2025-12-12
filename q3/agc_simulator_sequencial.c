
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h> 
#include <time.h>

// --- ESTADO DO SATÉLITE ---
int bateria = 100;
int temperatura = 25;
char status_sistema[100] = "EM ORBITA (STANDBY)";
int simulacao_rodando = 1;

// Função para limpar o lixo que você digitou enquanto o programa estava travado
void limpar_buffer_teclado() {
    // Tenta ler tudo o que está pendente no stdin e joga fora
    // (Nota: Em C padrão é difícil limpar stdin de forma portável, 
    // mas fseek ou loops de getchar funcionam em muitos casos interativos simples)
    fseek(stdin, 0, SEEK_END);
}

// --- FUNÇÃO DE SIMULAÇÃO DE TEMPO ---
void passar_tempo(int segundos, char* atividade) {
    for(int i = 1; i <= segundos; i++) {
        sleep(1); // TRAVA O PROGRAMA
        
        if (bateria > 0) bateria--;
        
        if (strstr(atividade, "MANOBRA") != NULL) {
            temperatura += 2;
        } else {
            if (temperatura > 25) temperatura--;
        }

        // Feedback visual para mostrar que está vivo, mas ocupado
        printf("\r\033[K"); 
        printf("[SEQUENCIAL] Processando %s... (%ds/%ds) Temp:%dC Bat:%d%%", atividade, i, segundos, temperatura, bateria);
        fflush(stdout);

        // Se a bateria morrer NO MEIO da ação, interrompe forçado
        if (bateria <= 0) {
            printf("\n\n\033[1;31m[FALHA DE SISTEMA] BATERIA ESGOTADA DURANTE OPERACAO!\033[0m\n");
            strcpy(status_sistema, "SISTEMA DESLIGADO (SEM ENERGIA)");
            return; 
        }
    }
    printf("\n");
}

// --- AÇÕES ---
void executar_manobra(int duracao) {
    if (bateria <= 0) {
        printf("\n\033[1;31m[ERRO] Sem energia para manobras.\033[0m\n");
        return;
    }
    sprintf(status_sistema, "EXECUTANDO MANOBRA (%ds)", duracao);
    printf("\n\033[1;34m[SISTEMA] Iniciando propulsores.\033[0m\n");
    
    passar_tempo(duracao, "MANOBRA"); 

    if (bateria > 0) {
        strcpy(status_sistema, "ORBITA ESTAVEL");
        printf("\033[1;34m[SISTEMA] Manobra finalizada.\033[0m\n");
    }
    
    // TRUQUE: Joga fora tudo o que você digitou enquanto esperava
    limpar_buffer_teclado(); 
}

void executar_downlink() {
    if (bateria <= 0) {
        printf("\n\033[1;31m[ERRO] Sem energia para rádio.\033[0m\n");
        return;
    }
    printf("\n\033[1;35m[RADIO] Iniciando transmissão...\033[0m\n");
    
    for(int i = 20; i <= 100; i += 20) {
        if (bateria <= 0) break; // Aborta se bateria morrer
        passar_tempo(2, "TRANSMISSAO");
        printf("\033[1;35m[RADIO] Pacote enviado: %d%%\033[0m\n", i);
    }
    
    if (bateria > 0) printf("\033[1;35m[RADIO] Download concluído.\033[0m\n");
    else printf("\033[1;31m[RADIO] Transmissão falhou (Bateria Morta).\033[0m\n");

    limpar_buffer_teclado();
}

void imprimir_interface() {
    printf("-------------------------------------------------\n");
    printf("   AGC - VERSAO SEQUENCIAL                       \n");
    printf("-------------------------------------------------\n");
    
    // Mostra status diferente se bateria estiver morta
    if (bateria <= 0) 
        printf(" BATERIA: %d%%  |  TEMP: %d C  |  STATUS: \033[1;31mMORTO\033[0m\n", bateria, temperatura);
    else
        printf(" BATERIA: %d%%  |  TEMP: %d C  |  STATUS: %s\n", bateria, temperatura, status_sistema);
    
    printf("-------------------------------------------------\n");
}

int main() {
    char comando[50];
    int duracao_manobra = 15; 

    printf("Iniciando Sistema...\n");

    while (simulacao_rodando) {
        imprimir_interface();
        printf("COMANDOS: [ORBITA] [BAIXAR] [CARREGAR] [STATUS] [SAIR]\n");
        printf("COMANDO > ");
        
        if (fgets(comando, sizeof(comando), stdin) != NULL) {
            comando[strcspn(comando, "\n")] = 0; 

            // VERIFICAÇÃO DE ENERGIA ANTES DE QUALQUER COISA
            if (bateria <= 0 && strcmp(comando, "CARREGAR") != 0 && strcmp(comando, "SAIR") != 0) {
                printf("\n\033[1;31m[ERRO CRITICO] SISTEMA SEM ENERGIA. RECARREGUE IMEDIATAMENTE.\033[0m\n");
                sleep(2); // Pequena pausa pra ler o erro
                continue; // Pula pro próximo loop, ignora o comando
            }

            if (strcmp(comando, "ORBITA") == 0) {
                executar_manobra(duracao_manobra);
            }
            else if (strcmp(comando, "BAIXAR") == 0) {
                executar_downlink();
            }
            else if (strcmp(comando, "CARREGAR") == 0) {
                bateria = 100;
                strcpy(status_sistema, "EM ORBITA (STANDBY)"); // Revive o sistema
                printf("\n\033[1;32m[SISTEMA] Bateria recarregada manualmente.\033[0m\n");
                sleep(1);
            }

            else if (strcmp(comando, "STATUS") == 0) {
                imprimir_interface(); 
            }
            else if (strcmp(comando, "SAIR") == 0) {
                simulacao_rodando = 0;
            }
            else {
                printf("Comando desconhecido.\n");
            }
        }
        printf("\n");
    }

}