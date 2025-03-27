#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "lwip/tcp.h"
#include "lwip/tcpip.h"
#include "ssd1306.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "C:\Users\juuze\bin2c\smile.h"
#include "C:\Users\juuze\bin2c\smile1.h"
#include <math.h>
#include "saveSystem.h"
#include "hardware/timer.h"

#define T1 0.82900
#define TAX 0.31
#define SSID "Familia Brandao "
#define PASS "994706949"
#define SERVER_IP "192.168.18.142"  // IP correto do servidor Flask
#define SERVER_PORT 5000
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C // Endereço I2C do display
#define I2C_SDA 14          // Pino SDA
#define I2C_SCL 15          // Pino SCL
#define LED_PIN 12          // Define o pino do LED
#define BTN_A_PIN 5 // Pino do botão A
#define BTN_B_PIN 6 // Pino do botão B
#define BTN_ANALOG_PIN 22 //Pino do botão do analogico
// Tamanho da imagem em bytes (para exibição no display)
#define Tamanho_da_imagem (128 * 64 / 8)
// Configuração do ADC para leitura do sensor SCT-013
#define ADC_PIN_SENSOR 28 // Pino do ADC onde o sensor está conectado
#define VREF 3.3 // Tensão de referência do ADC (em volts)
#define ADC_MAX 4095 // Valor máximo do ADC de 12 bits
// Parâmetros do sensor SCT-013
#define BURDEN_RESISTOR 33.0 // Resistor de carga em ohms para conversão de corrente
#define VOLTAGE_RMS 220.0 // Tensão RMS da rede elétrica (em volts)
#define SAMPLE_SIZE 500 // Número de amostras para cálculo RMS
#define CALIBRATION_FACTOR 1300 // Fator de calibração para conversão da leitura do ADC em corrente real
struct tcp_pcb *pcb;
struct data_t{
    float potency;
    float estime;
    float total;
};
// Instância do display SSD1306
ssd1306_t display;
float val;
// Variáveis de energia consumida
float energycon = 0.0;
int med = 0;
float potmid = 0.0;
float pottotal = 0.0;
double estimate = 0.0;
float potencia;

// Função para calcular o valor da conta de energia com impostos
float energybill(float watt){
    double cust = 0, trib = 0, subtotal = 0;
    cust = watt * T1;
    trib = cust * TAX;
    subtotal = cust + trib;
    return subtotal;
}

err_t tcp_connected_callback(void *arg, struct tcp_pcb *pcb, err_t err) {
    if (err == ERR_OK) {
        printf("Conexão estabelecida!\n");
        struct data_t *data = (struct data_t *)arg;
        // Criar requisição HTTP
        char request[512];
        float temperature = *(float *)arg; // Obtendo temperatura passada como argumento
        snprintf(request, sizeof(request), 
                 "GET /update?kw=%.2f&estimate=%.2f&total=%.4f HTTP/1.1\r\nHost: " SERVER_IP ":%d\r\n\r\n", 
                 data->potency, data->estime, data->total, SERVER_PORT);

        // Enviar dados
        err_t write_err = tcp_write(pcb, request, strlen(request), TCP_WRITE_FLAG_COPY);
        if (write_err == ERR_OK) {
            printf("Requisição enviada\n");
            tcp_output(pcb);
        } else {
            printf("Erro ao enviar requisição: %d\n", write_err);
        }

        // Fechar conexão após envio dos dados
        tcp_close(pcb);
        free(data);
        printf("Dado enviado e conexão fechada\n");
    } else {
        printf("Erro na conexão do servidor: %d\n", err);
        free(arg);    
    }
    return err;
}


void gpio_callback(uint gpio, uint32_t events){
    if(gpio == BTN_ANALOG_PIN && (events & GPIO_IRQ_EDGE_FALL)){
        printf("Botão pressionado\n");
        clearSaveData();    
    }

    if(gpio == BTN_ANALOG_PIN && (events & GPIO_IRQ_EDGE_RISE)){
        printf("Botão Solto!\n");
    }

}

bool timer_callback(repeating_timer_t *rt){
    write_float_to_flash(val);
    printf("SALVO na memoria = %f\n", val);
    return true;

}

void send_potencia(float potency, float estimate, float total) {
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb) { 
        printf("Erro ao criar PCB\n");
        return;
    }

    ip_addr_t server_ip;
    IP4_ADDR(&server_ip, 192,168,18,142); // Definir IP do servidor

    struct data_t *data = malloc(sizeof(struct data_t));
    if (!data){
        printf("Erro ao alocar memoria\n");
        return;
    }
    data->potency = potency;
    data->estime = estimate;
    data->total = total;

    // Passando o valor da potência como argumento da conexão
    tcp_arg(pcb, data);
    err_t err = tcp_connect(pcb, &server_ip, SERVER_PORT, tcp_connected_callback);
    if (err != ERR_OK) {  
        printf("Erro ao conectar ao servidor: %d\n", err);
        free(data);
        tcp_close(pcb);
        return;
    }

    printf("Tentando conectar ao Servidor... %d\n", err);

    sleep_ms(1000);
}

int calibrateADCoffset(int samples) {
    int sum = 0;
    int values[samples];

    for (int i = 0; i < samples; i++) {
        values[i] = adc_read();
        sum += values[i];
        sleep_us(1000);
    }

    int mean = sum / samples;

    float variance = 0;
    for (int i = 0; i < samples; i++) {
        variance += pow(values[i] - mean, 2);
    }
    variance /= samples;
    float std_dev = sqrt(variance);

    sum = 0;
    int count = 0;
    for (int i = 0; i < samples; i++) {
        if (fabs(values[i] - mean) <= (2 * std_dev)) {
            sum += values[i];
            count++;
        }
    }

    return count > 0 ? sum / count : mean;
}


// Função para medir a corrente RMS
float readCurrentRMS(int samples, int adc_offset, int adc_pin) {
    adc_select_input(ADC_BASE_PIN);
    float sum = 0;
    for (int i = 0; i < samples; i++) {
        uint16_t raw = adc_read();
        int adjusted_raw = raw - adc_offset;

        if (adjusted_raw < 29) adjusted_raw = 0;

        float voltage = (adjusted_raw * VREF) / ADC_MAX;
        float current = (voltage / BURDEN_RESISTOR) * CALIBRATION_FACTOR;
        //printf("Adjusted Raw: %d, Voltage: %.3f\n", adjusted_raw, voltage);
        sum += current * current;
        sleep_ms(1);
    }
    return sqrt(sum / samples);
}


// Função para exibir dados no display
void mostrarDadosNaTela(char *corrente, char *potencia, char *consumo) {

    ssd1306_clear(&display);

    if(consumo == NULL){
        ssd1306_draw_string(&display, 0, 0, 1, corrente);
        ssd1306_draw_string(&display, 0, 20, 1, potencia);
        ssd1306_draw_string(&display, 0, 40, 1, "Pressione B / voltar");
    }

    else{
        ssd1306_draw_string(&display, 0, 0, 1, corrente);
        ssd1306_draw_string(&display, 0, 20, 1, potencia);
        ssd1306_draw_string(&display, 0, 40, 1, consumo);
    }
    ssd1306_show(&display);
}

// Função que imprime a logo no display
void showinit(ssd1306_t *display, const uint8_t *imagem, size_t tamanho, uint32_t tempo) {
    ssd1306_clear(display);
    ssd1306_bmp_show_image(display, imagem, tamanho);
    ssd1306_show(display);
    sleep_ms(tempo);
}    

// Estrutura para o Menu
typedef struct {
    char text[50];
} OptionText;

typedef struct {
    char header[50];
    OptionText options[2];
    int qntOptions;
} Menu;

void trocaMenu(Menu *newMenu);
Menu *actualMenu;
Menu mainMenu;
Menu op1;
Menu sensorMenu;

void trocaMenu(Menu *newMenu) {
    actualMenu = newMenu;
}

void gpio_menu(uint gpio, uint32_t events){
    if(gpio == BTN_A_PIN && (events & GPIO_IRQ_EDGE_FALL)){
        trocaMenu(&sensorMenu);

    }
    if(gpio == BTN_B_PIN && (events & GPIO_IRQ_EDGE_FALL)){
        if(actualMenu == &sensorMenu || actualMenu == &op1){
            trocaMenu(&mainMenu);
        }
        else{trocaMenu(&op1);}
    }
}
int main() {
    stdio_init_all();

    i2c_init(i2c1, 400 * 1000); // 400 kHz
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializa botões
    gpio_init(BTN_A_PIN);
    gpio_set_dir(BTN_A_PIN, GPIO_IN);
    gpio_pull_up(BTN_A_PIN);

    gpio_init(BTN_B_PIN);
    gpio_set_dir(BTN_B_PIN, GPIO_IN);
    gpio_pull_up(BTN_B_PIN);

    gpio_init(BTN_ANALOG_PIN);
    gpio_set_dir(BTN_ANALOG_PIN, GPIO_IN);
    gpio_pull_up(BTN_ANALOG_PIN);

    // Inicializa o display
    if (!ssd1306_init(&display, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_ADDRESS, i2c1)) { 
        printf("Falha ao inicializar o display SSD1306\n");
        return 1; // Sai do programa
    }

    if (cyw43_arch_init()) {
        printf("Erro ao iniciar Wi-Fi\n");
        return 1;
    }

    cyw43_arch_enable_sta_mode();

    printf("Conectando ao Wi-Fi...\n");
    if (cyw43_arch_wifi_connect_timeout_ms(SSID, PASS, CYW43_AUTH_WPA2_AES_PSK, 20000)) {
        printf("Erro ao conectar ao Wi-Fi\n");
        return 1;
    }
    printf("Wi-Fi conectado!\n");

    showinit(&display, smile, Tamanho_da_imagem, 1000);
    showinit(&display, smile1, Tamanho_da_imagem, 500);
    showinit(&display, smile, Tamanho_da_imagem, 2000);

    // Inicializar menus
    mainMenu = (Menu) {
        "SMILE MENU",
        {{"Consumo Atual - A"}, {"Estimativa Mensal - B"}},
        2
    };

    op1 = (Menu) {
        "Op1",
        {{"Valor real"}, {"Voltar"}},
        2
    };

    sensorMenu = (Menu) {
        "Sensores",
        {{"Valor Y"}, {"Valor X"}},
        2
    };
    actualMenu = &mainMenu;

    // Inicializa o ADC
    adc_init();
    adc_gpio_init(ADC_PIN_SENSOR);
    adc_select_input(2);  // Seleciona o canal do sensor SCT-013 (Canal 1)

    // Calibração dinâmica do offset do ADC
    int adc_offset = calibrateADCoffset(500);
    printf("Offset do ADC calibrado: %d\n", adc_offset);

    ssd1306_clear(&display);

    repeating_timer_t timer;
    add_repeating_timer_ms(15000, timer_callback, NULL, &timer);
    float flashsave = read_float_from_flash();

    gpio_set_irq_enabled_with_callback(BTN_A_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_menu);
    gpio_set_irq_enabled_with_callback(BTN_B_PIN, GPIO_IRQ_EDGE_FALL | GPIO_IRQ_EDGE_RISE, true, &gpio_menu);

    if(flashsave <= 0){
        flashsave = 0;
    }

    energycon = flashsave;
    
    while (true) {
        med++;
        float corrente = readCurrentRMS(SAMPLE_SIZE, adc_offset, ADC_PIN_SENSOR);
        potencia = corrente * VOLTAGE_RMS;
        // Atualiza o valor de energia consumida
        float hours = 1.0 / 3600.0;
        energycon += (potencia * hours) / 1000;  // Acumula o valor consumido
        pottotal += potencia; // Potência total
        potmid = pottotal / (med > 0 ? med : 1); // Potência Média
        estimate = (potmid * 24 * 30) / 1000.0; // Potência Estimada
        send_potencia(potencia, estimate, energycon);
        val = energycon;
        printf("Potência calculada: %.2f W\n", potencia);


        if(gpio_get(BTN_ANALOG_PIN) == 0){
            clearSaveData();
            sleep_ms(1000);
        }


        if (actualMenu == &sensorMenu) {
            char kw[20], kwh[20], RSkwh[20]; // kwh - Valor consumido em kw | RSkwh - Valor consumido em Reais
            snprintf(kwh, sizeof(kwh), "Consumo:%.2f KWh", energycon);
            float energylive = energybill (energycon);
            sprintf(RSkwh, "R$ %.2f Reais", energylive);
            sprintf(kw, "Consumo:%.2f W", potencia);
            
            mostrarDadosNaTela(kw, kwh, RSkwh);
            
            continue; // Pular a lógica do menu normal e voltar ao loop
        }
        
        if (actualMenu == &op1) {
            char avarengeKW[20], avarengeRS[20]; // avarengeKW - Valor consumido em kw | avarengeRS - Valor consumido em Reais
            snprintf(avarengeKW, sizeof(avarengeKW), "Media KWh = %.2f", estimate);
            float energymens = energybill(estimate);
            snprintf(avarengeRS, sizeof(avarengeRS), "Media em R$: %.2f", energymens);
            
            mostrarDadosNaTela(avarengeKW, avarengeRS, NULL);

            continue; // Pular a lógica do menu normal e voltar ao loop
        }
        
        ssd1306_clear(&display);
        // Desenha o cabeçalho do menu
        ssd1306_draw_string(&display, 34, 0, 1, actualMenu->header);
        
        // Desenhar opções
        for (int i = 0; i < actualMenu->qntOptions; i++) {
            int _x = 0;
            int _y = 23 + i * 12;
            ssd1306_draw_string(&display, _x, _y, 1, actualMenu->options[i].text);
        }
        ssd1306_show(&display);
        
        
        tight_loop_contents();
    }
    return 0;
}
