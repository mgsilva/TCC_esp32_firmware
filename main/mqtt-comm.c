#include "mqtt-comm.h"
#include "wifi.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"

#define MQTT_TAG        "MQTT"
#define IP_BROKER  "192.168.1.9"
// #define MY_BROKER  "mqtt://matheusserver.ddns.net/"  /* -> Utilizar quando acessar via ddns o servidor
//                                                       *   Pode ser substituido pelo endereço de qualquer
//                                                       *   servidor MQTT de teste, como o "mqtt://mqtt.eclipse.org"
//                                                       */
extern QueueHandle_t send_data_q;
char message_to_send[1000];
esp_mqtt_client_handle_t client;

/*
 * Callback para tratar os eventos relacionados ao MQTT
 * Como os ESP32 só posta dados no servidor, nenhum evento é tratado aqui
 * e apenas a conexão é sinalizada
 */
void mqtt_event_callback(esp_mqtt_event_handle_t event){
    client = event->client;

    switch(event->event_id){
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(MQTT_TAG, "Conectou ao broker");    
            break;
        default:
            break;
    }
}
// Envia mensagem ao broker, no tópico selecionado
void send_message(char *message_to_send){
    int msg_id;
    msg_id = esp_mqtt_client_publish(client, "/topic/tcc-iot", message_to_send,\
                                            100004, 0, 0); 
}

//Função que recebe os eventos proveniente do loop de eventos.
//Chama uma função de callback para tratar esses eventos
static void mqtt_event_handler(void *event_handler_arg, \
            esp_event_base_t event_base,int32_t event_id, void *event_data){
    mqtt_event_callback(event_data);
}

/* Task para comunicação MQTT
 * A task fica esperando que a aquisição seja finalizada, para então enviar os dados
 */
void MQTT_task_to_send_data(){
    uint8_t *data_to_send = NULL;
    while (true){
        if(send_data_q != NULL){
            if(xQueueReceive(send_data_q, &data_to_send, portMAX_DELAY)){
                send_message((char *)data_to_send);
            }
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void mqtt_start(){
    // Configurando o básico para funcionamento do esp como cliente mqtt
    esp_mqtt_client_config_t mqtt_config = {
        .host = IP_BROKER,
        .port = 1883, //Porta que o broker mqtt está ouvindo
    };
    // Cria um manipulador para o cliente mqtt
    client = esp_mqtt_client_init(&mqtt_config);
    // Registra os eventos que serão observados dentro da API mqtt
    // Por padrão, MQTT_EVENT_ANY trata todos os eventos. 
    esp_mqtt_client_register_event(client, MQTT_EVENT_ANY, mqtt_event_handler, client);
    //Inicia o cliente MQTT com as configurações definidas
    esp_mqtt_client_start(client);

    xTaskCreate(MQTT_task_to_send_data, "mqtt-task", 4096, NULL, 5, NULL);
}