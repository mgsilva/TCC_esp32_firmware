#include "wifi.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//Bibliotecas do FreeRTOS para tratamento de eventos, 
//criação de task's e comunicação entre as tasks criadas
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
//Necessária para utilização do wifi
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#define TAG_WIFI "WIFI_DRIVER"

// Manipulador(Handle) do grupo de eventos do FreeRTOS, será utilizado para sinalizar conexão wifi
static EventGroupHandle_t wifi_event_group;

/* Vários bits podem ser utilizados para sinalizar 
 * a ocorrência de um evento. Como será utilizado apenas 
 * para sinalizar a conexão, ou falha de conexão, serão 
 * necessários apenas dois bits.
 */
#define WIFI_CONNECTED BIT0
#define WIFI_FAILED   BIT1
#define MAX_TRY       3

static int retry_num = 0;

/* Função de callback responsável por tratar os eventos do wifi.
 * Seguindo a documentação da API da Espressif, a função deve ser do tipo
 * typedefvoid (*esp_event_handler_t)(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
        esp_wifi_connect();
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED){
        if (retry_num < MAX_TRY){
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG_WIFI, "Tentando conectar de novo");
        } 
        else
            xEventGroupSetBits(wifi_event_group, WIFI_FAILED);
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP){
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG_WIFI, "Conseguiu IP:" IPSTR, IP2STR(&event->ip_info.ip));
        retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED);
    }

}
void wifi_init(){
    // Cria um grupo de eventos
    wifi_event_group = xEventGroupCreate();

    // Inicializa a pilha TCP/IP do ESP
    // Essa API fornece uma "abstração" das camadas mais baixas da pilha
    ESP_ERROR_CHECK(esp_netif_init());
    // Cria um loop de eventos, para monitorar a conexão wifi
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // Cria uma estação wifi padrão
    esp_netif_create_default_wifi_sta();
    /* Estrutura com parâmetros para configuração inicial do wifi (RX, TX, buffer size, potencia de sinal, ... etc).
     * A recomendação da Espressif é que a estrutura seja inicializada 
     * com valores padrões, para facilitar a compatibilidade do código
     * com versões futuras do framework.
     */
    wifi_init_config_t wifi_conf= WIFI_INIT_CONFIG_DEFAULT();    
    // Inicia o wifi
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_conf));

    /* Cria uma instancia para cada evento definido.
     * Toda vez que um dos eventos ocorrer, a função "wifi_event_handler" será chamada.
     * No caso do "WIFI_EVENT", qualquer evento de wifi acionara a função.
     */
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_any_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                    ESP_EVENT_ANY_ID,
                                                    &wifi_event_handler,
                                                    NULL,
                                                    &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                    IP_EVENT_STA_GOT_IP,
                                                    &wifi_event_handler,
                                                    NULL,
                                                    &instance_any_ip));
    // Estrutura para configuração do wifi, como ssid, senha, criptografia e autenticação
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID, //SSID da rede
            .password = WIFI_PASSWD, //Senha para conectar na rede
            .threshold.authmode = WIFI_AUTH_WPA2_PSK, //Tipo de autenticação (WPA2/PSK)
            // Configuração do PMF (protected managment frame), que evita que pacotes sejam
            // interceptados no interior de uma rede.
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    /* Define o driver wifi como station (estaçaõ wifi)
     * Configura o driver, com as configurações especificadas acima
     * e Inicia o driver.
     */
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG_WIFI, "Inicio!");

    /* Espera por tempo indeterminano (portMAX_DELAY)
     * Até que Wifi tenha conseguido conectar, ou tenha falhado em conectar.
     */
    EventBits_t wifi_status = xEventGroupWaitBits(wifi_event_group,
                                            WIFI_CONNECTED| WIFI_FAILED,
                                            pdFALSE,
                                            pdFALSE,
                                            portMAX_DELAY);
    //Testa qual evento aconteceu, para feedback
    if(wifi_status == WIFI_CONNECTED)
        ESP_LOGI(TAG_WIFI, "Wifi Conectou");
    else if(wifi_status == WIFI_FAILED)
        ESP_LOGI(TAG_WIFI, "Wifi não conectou :(");
    else
        ESP_LOGI(TAG_WIFI, "Não sei oque aconteceu!");

    // "Desregistra" as instâncias criadas para tratar os eventos.
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_any_ip));
    ESP_ERROR_CHECK(esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id));
    // Deleta o grupo de eventos criado para tratar wifi
    vEventGroupDelete(wifi_event_group);
}

void flash_mem_init(){
    /* Iniciando a memoria flash.
     * È necessário fazer isso pois a API de Wi-fi da Espressif
     * utiliza a memoria flash para salvar valores importantes
     * como SSID e senha da rede
     */
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK( ret );
}