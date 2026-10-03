#!/bin/bash

# 1. Verifica se o usuário informou o nome do arquivo para o cliente
if [ -z "$1" ]; then
    echo "Use: $0 <filename>"
    exit 1
fi

ARQUIVO="$1"
if [[ "$ARQUIVO" != /* ]]; then
    ARQUIVO="$PWD/$ARQUIVO"
fi
cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." || exit 1

echo "Compilando o projeto..."
make server client

if [ $? -ne 0 ]; then
    echo "Erro na compilação. Abortando."
    exit 1
fi

echo "Iniciando o servidor..."
./bin/server &

# Salva o ID do processo do servidor para encerrá-lo depois
SERVER_PID=$!
trap 'kill "$SERVER_PID" 2>/dev/null; wait "$SERVER_PID" 2>/dev/null' EXIT

# Aguarda 1 segundo para o servidor iniciar
sleep 1
if ! kill -0 "$SERVER_PID" 2>/dev/null; then
    echo "Erro ao iniciar o servidor."
    exit 1
fi

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
    ./bin/client "$ARQUIVO"
    echo "---------------------------------------------"
done

# 5. Limpeza ao sair do loop
echo "Finalizando o servidor (PID: $SERVER_PID)..."
echo "Feito!"
