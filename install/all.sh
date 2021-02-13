#!/bin/bash

if [[ $UID != 0 ]]; then
    echo "Por favor, rode o script como super-usuário:"
    echo "sudo $0 $*"
    exit 1
fi

echo "Atualizando todos os pacotes..."
apt-get update > /dev/null
apt-get upgrade > /dev/null
echo "Rodando script de instalação do nodejs..."
./node.sh
echo "Rodando script de instalação do influxdb..."
./influxdb.sh
echo "Rodando script de instalação do grafana..."
./grafana.sh
echo "Rodando script de instalação do broker mosquitto..."
./mqtt.sh