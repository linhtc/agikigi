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
#include "freertos/queue.h"
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
#include "driver/adc.h"

/*Add websocket lib*/
#include "websocket.h"

/*Include temperature lib*/
#include "ds18b20.h"

/*Include ultrasonic lib*/
#include "hcsr04.h"

/*Include ph lib*/
#include "ph20.h"

/*Include dissolved oxygen lib*/
#include "do37.h"

/*Define temperature pin and storage*/
const int DS_PIN = 14;
float VAR_TEMPERATURE = 0;

/*Define ultrasonic sensor pin and storage*/
const int HC_TRIG = 18;
const int HC_ECHO = 19;
double VAR_DISTANCE = 0;

/*Define PH sensor pin and storage*/
double VAR_PH = 0;

/*Define DO sensor pin and storage*/
double VAR_DO = 0;

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

/*
 * Event handler
 *
 * */
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

/*
 * Setup wifi
 *
 * */
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

/*
 * Setup ultrasonic sensor
 *
 * */
static void initialise_ultrasonic(void)
{
	gpio_pad_select_gpio(HC_TRIG);
	gpio_pad_select_gpio(HC_ECHO);
	gpio_set_direction(HC_TRIG, GPIO_MODE_OUTPUT);
	gpio_set_direction(HC_ECHO, GPIO_MODE_INPUT);
}

/*
 * Queue of web socket
 *
 * */
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
        //ESP_LOGI(TAG, "Connected to AP");

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
						case 1:{ /*Get water meter*/
							cJSON_AddNumberToObject(response, "te_m", VAR_TEMPERATURE); /*Temperature meter*/
							cJSON_AddNumberToObject(response, "di_m", VAR_DISTANCE); /*Distance meter*/
							cJSON_AddNumberToObject(response, "ph_m", VAR_PH); /*PH meter*/
							cJSON_AddNumberToObject(response, "do_m", VAR_DO); /*DO meter*/
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

/*
 * Read temperature
 * Sensor DS18B20 waterproof
 * GPIO 14
 *
 * */
static void temperature(void *pvParameters)
{
	ds18b20_init(DS_PIN);
	while (1) {
		VAR_TEMPERATURE = ds18b20_get_temp();
	    printf("Temperature: %0.1f C\n", VAR_TEMPERATURE);
	    vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

/*
 * Read distance from ultrasonic sensor
 * Sensor HC SR04
 * GPIO 18 -> trigger
 * GPIO 19 -> echo
 *
 * */
static void distance(void *pvParameters)
{
	hcsr04_init(HC_TRIG, HC_ECHO);
	while (1) {
		VAR_DISTANCE = hcsr04_get_distance();
		printf("Distance: %0.1f Cm\n", VAR_DISTANCE);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

/*
 * Read ph from ph sensor
 * Sensor PH 2.0
 * GPIO 36 -> ADC1_CHANNEL_0 -> DB_11 3.3v
 *
 * */
static void ph_meter(void *pvParameters)
{
	ph20_init(ADC1_CHANNEL_0, ADC_WIDTH_MAX, ADC_ATTEN_DB_11);
	while (1) {
		VAR_PH = ph20_get_meter();
		printf("PH: %0.1f U\n", VAR_PH);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

/*
 * Read dissolved oxygen from do sensor
 * Sensor DO
 * GPIO 39 -> ADC1_CHANNEL_3 -> DB_11 3.3v
 *
 * */
static void do_meter(void *pvParameters)
{
	ph20_init(ADC1_CHANNEL_3, ADC_WIDTH_MAX, ADC_ATTEN_DB_11);
	while (1) {
		VAR_DO = do37_get_meter();
		printf("DO: %0.1f mg/l\n", VAR_DO);
		vTaskDelay(1000 / portTICK_PERIOD_MS);
	}
}

/*
 * Main function
 * Pin tasks to core 0, 1
 *
 * */
void app_main()
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    initialise_wifi();
    initialise_ultrasonic();
    xTaskCreate(&ws_server, "ws_server", 2048, NULL, 4, NULL);
    xTaskCreatePinnedToCore(&waiting_req, "waiting_req", 2048, NULL, 5, NULL, 1);
    xTaskCreatePinnedToCore(&temperature, "temperature", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&distance, "distance", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&ph_meter, "ph_meter", 2048, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(&do_meter, "do_meter", 2048, NULL, 5, NULL, 0);
}
