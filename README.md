# PROJETO SMILE (Sistema de Monitoramento Inteligente de Energia)
Projeto SMILE, desenvolvido para o projeto final do Embarcatech, tem como objetivo monitorar o consumo de energia e notificar o usuário acerca dos gastos.

[Documentação completa](https://docs.google.com/document/d/1gVZOIpe5HDg1jDSerwmRkDiQZjkOWxqEv-SfY-Xyfz4/edit?tab=t.0)

## Sumário
- [Descrição do Funcionamento](#descrição-do-funcionamento)
  - [Interface](#interface)
- [Especificação do Hardware](#especificação-do-hardware)
  - [Componentes Utilizados](#componentes-utilizados)
  - [Pinagem dos Componentes](#pinagem-dos-componentes)
- [Especificação do Firmware](#especificação-do-firmware)
  - [Inicialização dos Periféricos](#inicialização-dos-periféricos)
  - [Leitura e Processamento dos Dados](#leitura-e-processamento-dos-dados)
  - [Interface de Usuário](#interface-de-usuário)
- [Endpoints do Servidor](#endpoints-do-servidor)
- [Melhorias Futuras](#melhorias-futuras)


## Descrição do funcionamento
O SMILE é um dispositivo criado na placa BitDogLab, usando um sensor de corrente o SCT-013 que ler a corrente da rede e por meio do codigo faz as conversões em corrente e potência, para depois realizar o envio dos dados ao computador de borda e para o display. Havendo duas opções "Consumo Atual" e "Estimativa Mensal", mostrando informações variadas como previsão de gastos e consumo em tempo real.
### Interface
- **Consumo Atual:**
Realiza as leituras em tempo real da potencia em KW, consumo em KWH e estimativa em reais.
- **Estimativa Mensal**
Por meio de um calculo de média do consumo, nessa opção é feita a estimativa de gasto no mês, tanto em KWH como em Reais.
## Especificação do Hardware
### Componentes Utilizados
- **Raspberry Pi Pico W**
Microcontrolador com um processador Dual-Core ARM Cortex M0+, rodando em até 133MHz. Possui 264kB de memória SRAM e 2MB de memória Flash integrada na placa. Além de conexões GPIO para conexão de sensores ou atuadores, gerenciamento de interrupções, conectividade WI-FI e Bluetooth, suporte a UART, SPI, I2C, ADC e PIO.
- **Sensor SCT-013**
O Sensor de Corrente Não Invasivo SCT-013-000 é um equipamento para medir corrente até 100A e que não é invasivo. Sem necessidade de cortes de fios para remendos, apenas colocando o fio entre a sua “garra”. 
- **Display OLED SSD1306**
O Display OLED 128x32 0.91 é um display pequeno mas com grande nitidez e baixo consumo de energia. Esse display tem luz própria, não sendo necessária a utilização de Backlight.
- **Botão**
O Push button (botão de pressão) é uma chave que, quando pressionado o botão, ela abre ou fecha o circuito, convertendo assim, um comando mecânico em elétrico. Geralmente eles tem um contato de ação momentânea, abrindo ou fechando o circuito apenas de modo momentâneo.
### Pinagem dos componentes
| Componente           | Pino no Raspberry Pi Pico W (BitDogLab) | Função                                 |
|----------------------|-----------------------------------------|----------------------------------------|
| Botão A              | GPIO 5                                  | Navegar entre os Menus                 |
| Botão B              | GPIO 6                                  | Navegar entre os Menus                 |
| Botão C              | GPIO 22                                 | Reset da Memória                       |
| Sensor SCT-013       | GPIO 28                                 | Medir Corrente                         |
| Display OLED (SDA)   | GPIO 14                                 | Comunicação I2C – Dados                |
| Display OLED (SCL)   | GPIO 15                                 | Comunicação I2C – Clock                |
## Especificação do Firmware

O firmware foi desenvolvido em linguagem C, utilizando o SDK oficial do Raspberry Pi Pico W e a biblioteca pico-ssd1306 para controle do display OLED. Suas principais funcionalidades incluem:

### Inicialização dos Periféricos

Logo após a energização do sistema, o firmware executa a inicialização dos seguintes componentes:

- Botões de navegação: Configurados nos pinos GPIO 5 e 6.

- Barramento I2C: Configurado nos pinos SDA (14) e SCL (15) para comunicação com o display OLED.

- Display OLED SSD1306: Inicialização e exibição de telas iniciais.

- ADC: Configuração do ADC para leitura do sensor SCT-013 no pino 28.

- WI-FI: Conectando a rede local e ao computador de borda.

### Leitura e Processamento dos Dados**

#### 1. Leitura dos Dados do Sensor  
- O firmware usa o **ADC (Conversor Analógico-Digital)** do Raspberry Pi Pico para ler os dados do sensor de corrente SCT-013.  
- O **pino ADC 28** é utilizado para a conexão com o sensor.  
- O ADC é configurado para operar com uma **tensão de referência de 3.3V** e uma resolução de **12 bits (0-4095)**.  

#### 2. Calibração do Sensor  
- Antes de iniciar a medição, o firmware executa uma **calibração dinâmica do offset do ADC** para minimizar ruídos e variações.  
- A calibração remove **valores anômalos** usando um cálculo de **média e desvio padrão**.  

#### 3. Cálculo da Corrente RMS  
- O firmware realiza múltiplas leituras (500 amostras por padrão).  
- Cada leitura é convertida para tensão usando a fórmula:  
  ```
  V = (Leitura_ADC * V_REF) / ADC_MAX
  ```  
- A corrente é calculada com base na resistência de carga do circuito:  
  ```
  I = (V / BURDEN_RESISTOR) * CALIBRATION_FACTOR
  ```  
- Para obter o **valor RMS**, a soma dos quadrados das correntes medidas é dividida pelo número total de amostras e depois tiramos a raiz quadrada.  

#### 4. Cálculo da Potência e Consumo de Energia  
- A potência instantânea é calculada pela fórmula:  
  ```
  P = I_RMS * V_RMS
  ```  
- O consumo de energia acumulado é atualizado a cada ciclo:  
  ```
  Energia += (P * tempo_de_amostragem) / 1000
  ```  
- A média da potência é usada para **estimar o consumo mensal**.  

#### 5. Armazenamento dos Dados  
- O consumo de energia é salvo na memória flash a cada 6 segundos para **evitar perda de dados** após reinicializações.  
- O usuário pode **resetar o consumo acumulado** pressionando um Botão C.  

#### 6. Envio dos Dados para o Servidor  
- A cada leitura, os dados são enviados para um **servidor Flask via requisição HTTP GET**.  
- O envio inclui:  
  - Potência atual  
  - Estimativa mensal de consumo  
  - Total de energia consumida  
- A comunicação é feita via **Wi-Fi (CYW43)**/**TCP/IP**/ e usa **IP fixo** para o servidor.  


## Interface de Usuário

A interface do firmware utiliza menus exibidos no display OLED e navegáveis pelos botões A e B:

- Botão A: Alterna entre os modos de medição.
- Botão B: Exibe logs e estatísticas do consumo de energia.
- Botão C: Reseter Mémoria.
  
Os menus disponíveis incluem:

- Menu Principal: Apresenta as opções de medição.
- Modo Sensores: Exibe a corrente, potência e consumo acumulado.
- Modo Estimativa: Mostra a média de consumo mensal e o custo aproximado.
## Endpoints do Servidor
- **`GET /update?kw=<valor>&estimate=<valor>&total=<valor>`** → Atualiza os dados de consumo no servidor.

## Melhorias Futuras
- Integração com sistemas de automação residencial.

## Autor:
Desenvolvedor do Projeto Smile: Antonio José Mota Ferreira Brandão
