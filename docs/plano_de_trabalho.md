# Plano de Trabalho - PJI 3

1. **Identificação da proposta**

    1. **Título**

        Sistema de monitoração de equipamentos de refrigeração utilizando IoT

    2. **Prazo**

        `23/04/2021`

    3. **Equipe**

        * Eduarda Passig e Silva
        * Fabiano Kraemer
        * Guilherme Fleiger Felipe

    4. **Resumo da proposta**

        Monitoração e análise dos dados para indicação de estabilidade de um sistema de refrigeração, indicando períodos para manutenção preventiva e/ou problemas no equipamento, desligando-o caso necessário. Informações transmitidas sem fio para um navegador/aplicativo para consulta e controle pelo usuário.

    5. **Data de início do projeto**

        `30/11/2020`
    
2. **Descrição da proposta**

    1. **Justificativa**

        Populações em rincões remotos no Brasil, como os ribeirinhos, costumam viver em regiões onde não possuem infraestrutura elétrica, tendo dificuldades em coisas aparentemente simples para nossa realidade local, como manter alimentos conservados/refrigerados. Uma alternativa seria disponibilizar equipamentos alimentados por energias renováveis, como eólica e solar. Mas essas energias são intermitentes, e sistemas com baterias para armazenar a energia ainda são muito caros, ainda mais para a realidade normalmente pobre dessas populações.

        A área de RAC do campus São José possui um projeto de um sistema refrigerador alimentado por energia solar, onde se analisa a possibilidade de armazenar a energia já diretamente transformada em “frio”, suportando períodos noturnos e/ou com baixo índice solar. Mas, além do problema de armazenar essa energia, o sistema precisa funcionar continuamente por longos períodos, sem falhar, pois uma falha para ser arrumada poderá levar dias ou até semanas.

        Nossa proposta entra nesse ponto. Através de diversos sensores, o sistema será monitorado continuamente para que através da análise dos dados lidos dos mesmos, seja indicado para um operador humano uma possível falha ou indicador de futura falha, informando para o operador a necessidade de uma manutenção preventiva.

    2. **Objetivo geral**

        Agrupar dados e utilizar aprendizado de máquina para prever possíveis danos a equipamentos de refrigeração, visando reduzir custo de mão de obra e tempo em que determinado equipamento fica sem funcionamento. 

        Todo o sistema de monitoração funcionará de forma isolada e alimentada por energia solar, visando o funcionamento em regiões afastadas e sem conexão a energia elétrica/internet. O sistema de monitoração em vários equipamentos se comunicará com um único servidor central (também fornecido pelo projeto) que fará a predição e dará os avisos necessários às pessoas competentes pela mão de obra de reparo.

    3. **Objetivos específicos**

        Ler diversos sensores em um sistema de refrigeração, a fim de obter dados que possam ser processados por um algoritmo e prever um comportamento anômalo do sistema, informando a necessidade de manutenção/observação antes que um problema de fato ocorra.

    4. **Delimitação/Restrições**

        - Quarentena devido ao coronavírus.
        - Falta de conexão com rede elétrica e internet.
        - Equipamentos fotovoltaicos para suprir energia para o sistema.
        - Instrumentos adequados para validação dos sensores (osciloscópios, fontes, etc).

3. **Materiais e insumos previstos**

    * 1 - Raspberry pi 3B
    * 1 - ESP32 WROOM
    * 9 - sensores NTC digitais DS18B20
    * 2 - transdutores de pressão GTP 1000 Gulton  4mAh 20mAh
    * 1 - Wattímetro PZEM-004T-v30-master
    * 1 - acelerômetro ADXL345 (ainda não instalado e não disponível no código atual)
    * 1 - Exaustor 24V para o condensador
    * 1 - Relê de ligamento/desligamento do compressor
    * 1 - Sistema de refrigeração com compressor, 2 manômetros, evaporadora, condensadora, caixa de isopor
    * Cabos diversos para ligação dos sensores/sistema embarcado
