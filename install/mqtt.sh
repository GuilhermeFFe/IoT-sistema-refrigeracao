#!/bin/bash

if [[ $UID != 0 ]]; then
    echo "Por favor, rode o script como super-usuário:"
    echo "sudo $0 $*"
    exit 1
fi

if dpkg -s mosquitto > /dev/null 2>&1; then
    echo "O broker mosquitto já está instalado"
    exit 0
fi

echo "O broker mosquitto não está instalado, instalando..."
apt-get-install mosquitto > /dev/null