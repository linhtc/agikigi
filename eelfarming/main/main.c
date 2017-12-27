/* HTTP GET Example using plain POSIX sockets
   This example code is in the Public Domain (or CC0 licensed, at your option.)
   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "cJSON.h"
#include "WebSocket_Task.h"

#include "freertos/queue.h"

/*Include temperature lib*/
#include "ds18b20.h"

/*Define temperature pin*/
const int DS_PIN = 14;

/* The examples use simple WiFi configuration that you can set via
   'make menuconfig'.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_WIFI_SSID "Leon A.one"
#define EXAMPLE_WIFI_PASS "Leon@09131"

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;

/* The event group allows multiple bits for each event,
   but we only care about one event - are we connected
   to the AP with an IP? */
const int CONNECTED_BIT = BIT0;

static const char *TAG = "example";

//WebSocket frame receive queue
QueueHandle_t WebSocket_rx_queue;

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
    case SYSTEM_EVENT_STA_START:
        esp_wifi_connect();
        break;
    case SYSTEM_EVENT_STA_GOT_IP:
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        break;
    case SYSTEM_EVENT_STA_DISCONNECTED:
        /* This is a workaround as ESP32 WiFi libs don't currently
           auto-reassociate. */
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        break;
    default:
        break;
    }
    return ESP_OK;
}

static void initialise_wifi(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_storage(WIFI_STORAGE_RAM) );
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_WIFI_SSID,
            .password = EXAMPLE_WIFI_PASS,
        },
    };
    ESP_LOGI(TAG, "Setting WiFi configuration SSID %s...", wifi_config.sta.ssid);
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

static void waiting_req(void *pvParameters)
{
    //frame buffer
	WebSocket_frame_t __RX_frame;

	//create WebSocket RX Queue
	WebSocket_rx_queue = xQueueCreate(10,sizeof(WebSocket_frame_t));

    while(1) {
        /* Wait for the callback to set the CONNECTED_BIT in the
           event group.
        */
        xEventGroupWaitBits(wifi_event_group, CONNECTED_BIT, false, true, portMAX_DELAY);
//        ESP_LOGI(TAG, "Connected to AP");

        if(xQueueReceive(WebSocket_rx_queue,&__RX_frame, 3*portTICK_PERIOD_MS)==pdTRUE){
			//write frame inforamtion to UART
			printf("New Websocket frame. Length %d, payload %.*s \r\n", __RX_frame.payload_length, __RX_frame.payload_length, __RX_frame.payload);

			cJSON *socketQ = cJSON_Parse(__RX_frame.payload);
			if(socketQ != NULL){
				cJSON *response = cJSON_CreateObject();
				cJSON *cmd = cJSON_GetObjectItem(socketQ, "cmd");
				if(cmd != NULL){
					ESP_LOGI(TAG, "cmd --> %d", cmd->valueint);
					switch (cmd->valueint){ // 0 => ack, 1 -> info, 2 set ssid, 3 control pin
						case 0:{
							cJSON_AddNumberToObject(response, "status", 1);
							break;
						}
						default:{
							cJSON_AddNumberToObject(response, "status", 0);
							break;
						}
					}
				}
				char *res = cJSON_Print(response);
				esp_err_t err = WS_write_data(res, strlen(res));
				ESP_LOGI(TAG, "send %s -> %d", res, err);
			} else{
				//loop back frame
				WS_write_data(__RX_frame.payload, __RX_frame.payload_length);
			}

			//free memory
			if (__RX_frame.payload != NULL){
				free(__RX_frame.payload);
			}
		}
    }
}

static void temperature(void *pvParameters)
{
	ds18b20_init(DS_PIN);
	while (1) {
	    printf("Temperature: %0.1f\n", ds18b20_get_temp());
	    vTaskDelay(1000 / portTICK_PERIOD_MS);
	  }
}

void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    xTaskCreate(&ws_server, "ws_server", 2048, NULL, 4, NULL);
    xTaskCreatePinnedToCore(&waiting_req, "waiting_req", 2048, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&temperature, "mainTask", 2048, NULL, 5, NULL, 0);
}
