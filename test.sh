#!/bin/bash

# 1. Verifica se o usuário informou o nome do arquivo para o cliente
if [ -z "$1" ]; then
    echo "Use: $0 <filename>"
    exit 1
fi

ARQUIVO="$1"

echo "Compilando o projeto..."
make

if [ $? -ne 0 ]; then
    echo "Erro na compilação. Abortando."
    exit 1
fi

echo "Iniciando o servidor..."
./server &

# Salva o ID do processo do servidor para encerrá-lo depois
SERVER_PID=$!

# Aguarda 1 segundo para o servidor iniciar
sleep 1

# 4. Loop interativo para o cliente
while true; do
    echo ""
    echo "-> Pressione [ENTER] para executar o cliente."
    echo "-> Digite [q] (ou qualquer tecla + ENTER) para encerrar o servidor."
    echo -n "Escolha: "
    read -r opcao

    # Se o usuário digitou qualquer coisa, sai do loop
    if [ -n "$opcao" ]; then
        echo "Encerrando o ambiente..."
        break
    fi

    # Se o usuário apenas apertou ENTER (string vazia), roda o cliente
    echo "---------------------------------------------"
    echo "Executando o cliente com o arquivo: $ARQUIVO"
    echo "---------------------------------------------"
    ./client "$ARQUIVO"
    echo "---------------------------------------------"
done

# 5. Limpeza ao sair do loop
echo "Finalizando o servidor (PID: $SERVER_PID)..."
kill $SERVER_PID

echo "Feito!"

