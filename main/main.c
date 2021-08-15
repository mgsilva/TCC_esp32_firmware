#include <stdio.h>
#include <stdlib.h>
#include "wifi.h"
#include "mqtt-comm.h"
#include "adc-comm.h"

void app_main(){

    printf("TCC - Aluno: Matheus Goulart\nEste programa cria uma estação Wi-fi\n\r \
            para comunicação MQTT.\n\n");

    flash_mem_init();
    
    // Conecta ao wifi
    wifi_init();
    // Inicia comunicação MQTT
    mqtt_start();
    // Inicia aquisição de dados
    spi_init();

    vTaskDelete(NULL);
}
