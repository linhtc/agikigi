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

/*Add websocket lib*/
#include "websocket.h"

/*Include temperature lib*/
#include "ds18b20.h"

/*Define temperature pin*/
const int DS_PIN = 14;

/*Define ultrasonic sensor pin*/
const int HC_TRIG = 18;
const int HC_ECHO = 19;

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

/*Temperature storage*/
float VAR_TEMPERATURE = 0;

/*Distance storage*/
double VAR_DISTANCE = 0;
struct timeval tv = { .tv_sec = 0, .tv_usec = 0 };

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
						case 1:{ /*Get temperature*/
							cJSON_AddNumberToObject(response, "temperature", VAR_TEMPERATURE);
							break;
						}
						case 2:{ /*Get distance*/
							cJSON_AddNumberToObject(response, "distance", VAR_DISTANCE);
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
	    printf("Temperature: %0.1f\n", VAR_TEMPERATURE);
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
	while (1) {
		bool has_echo = false;
		gpio_set_level(HC_TRIG, 1);
		ets_delay_us(100);
		gpio_set_level(HC_TRIG, 0);
		gettimeofday(&tv, NULL);
		uint32_t startTime = tv.tv_usec;
		// Wait for echo to go high and THEN start the time
		while (gpio_get_level(HC_ECHO) == 0 && gettimeofday(&tv, NULL) && (tv.tv_usec - startTime) < 500 * 1000) {
			// Do nothing;
		}
		gettimeofday(&tv, NULL);
		startTime = tv.tv_usec;
		while (gpio_get_level(HC_ECHO) == 1 && gettimeofday(&tv, NULL) && (tv.tv_usec - startTime) < 500 * 1000) {
			has_echo = true;
		}
		if (gpio_get_level(HC_ECHO) == 0 && has_echo) {
			gettimeofday(&tv, NULL);
			uint32_t diff = tv.tv_usec - startTime; // Diff time in uSecs
			// Distance is TimeEchoInSeconds * SpeedOfSound / 2
			VAR_DISTANCE  = 340.29 * diff / (1000 * 1000 * 2); // Distance in meters
			printf("Distance is %f cm\n", VAR_DISTANCE * 100);
		} else {
			// No value
			printf("Distance: n/a\n");
		}
		// Delay and re run.
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
}
