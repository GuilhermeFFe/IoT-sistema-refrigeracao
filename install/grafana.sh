#!/bin/bash

if [[ $UID != 0 ]]; then
    echo "Por favor, rode o script como super-usuário:"
    echo "sudo $0 $*"
    exit 1
fi

if dpkg -s grafana > /dev/null 2>&1; then
    echo "O grafana já está instalado"
    exit 0
fi

echo "O grafana não está instalado, instalando..."
echo "Obtendo a chave pública do repositório..."
wget -qO- https://packages.grafana.com/gpg.key | sudo apt-key add -
echo "Adicionando o repositório debian buster do grafana..."
echo "deb https://packages.grafana.com/oss/deb stable main" | sudo tee -a /etc/apt/sources.list.d/grafana.list

echo "Instalando..."
apt-get update > /dev/null
apt-get install grafana > /dev/null

echo "Habilitando o serviço do grafana-server no systemd..."
systemctl enable grafana-server > /dev/null

echo "Iniciando o serviço..."
systemctl start grafana-server > /dev/null