#!/bin/bash
#chmod +x run_tests.sh
#chmod +x run_tests.sh
#./run_tests.sh >> resultados_experimento.txt
# --- 1. Compilação ---
echo "Compilando os programas..."
# Compila o Sequencial
gcc -std=c11 -Wall -O2 produto_sequencial.c -o prod_seq
# Compila o Paralelo
gcc -std=c11 -Wall -O2 -pthread produto_paralelo.c -o prod_par

# Verifica se compilou
if [[ ! -f "./prod_seq" ]] || [[ ! -f "./prod_par" ]]; then
    echo "Erro na compilação!"
    exit 1
fi

echo "Iniciando os testes..."

# --- 2. Definição dos Parâmetros ---
# Tamanhos do vetor
TAMANHOS=(500 1000 5000 10000)
# Quantidade de threads
THREADS=(4 8 16 32)

# --- 3. Execução ---
for size in "${TAMANHOS[@]}"; do
    
    # A) Executa o SEQUENCIAL uma vez para esse tamanho
    ./prod_seq $size

    # B) Executa o PARALELO para cada variação de thread
    for t in "${THREADS[@]}"; do
        ./prod_par $size $t
    done

done

echo "Testes finalizados!" 