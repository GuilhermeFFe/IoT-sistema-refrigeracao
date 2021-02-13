#!/bin/bash

if [[ $UID != 0 ]]; then
    echo "Por favor, rode o script como super-usuário:"
    echo "sudo $0 $*"
    exit 1
fi

if dpkg -s influxdb > /dev/null 2>&1; then
    echo "O influxdb já está instalado"
    exit 0
fi

echo "O influxdb não está instalado, instalando..."
echo "Obtendo a chave pública do repositório..."
wget -qO- https://repos.influxdata.com/influxdb.key | sudo apt-key add -
echo "Adicionando o repositório debian buster do influxdb..."
echo "deb https://repos.influxdata.com/debian buster stable" | sudo tee /etc/apt/sources.list.d/influxdb.list

echo "Instalando..."
apt-get update > /dev/null
apt-get install influxdb > /dev/null

echo "Habilitando o serviço do influxdb no systemd..."
systemctl unmask influxdb > /dev/null
systemctl enable influxdb > /dev/null

echo "Iniciando o serviço..."
systemctl start influxdb