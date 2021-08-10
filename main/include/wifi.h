#ifndef __WIFI_CONN__
#define __WIFI_CONN__

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


#define WIFI_SSID "Nome_da_rede_wifi"
#define WIFI_PASSWD "Senha_da_rede_wifi"


void flash_mem_init();

void wifi_init();

#endif
