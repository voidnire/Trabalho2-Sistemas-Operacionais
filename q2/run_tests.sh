#!/bin/bash
#chmod +x run_tests.sh
#chmod +x run_tests.sh
#./run_tests.sh >> resultados_experimento.txt
# --- 1. Compilação ---
echo "Compilando os programas..."
# Compila o Sequencial
gcc -std=c11 -Wall -O3 matriz_sequencial.c -o matseq
# Compila o Paralelo
gcc -std=c11 -Wall -O3 -pthread matriz_paralelo.c -o matpar

# Verifica se compilou
if [[ ! -f "./matseq" ]] || [[ ! -f "./matpar" ]]; then
    echo "Erro na compilação!"
    exit 1
fi

echo "Iniciando os testes..."

# --- 2. Definição dos Parâmetros ---
# Tamanhos do vetor
TAMANHOS=(500 1000 2000 3000)
# Quantidade de threads
THREADS=(4 8 16 32)

# --- 3. Execução ---
for size in "${TAMANHOS[@]}"; do
    
    # A) Executa o SEQUENCIAL uma vez para esse tamanho
    ./matseq $size

    # B) Executa o PARALELO para cada variação de thread
    for t in "${THREADS[@]}"; do
        ./matpar $size $t
    done

done

echo "Testes finalizados!" 