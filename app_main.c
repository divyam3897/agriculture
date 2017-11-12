#include "rom/ets_sys.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "sdkconfig.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt.h"

#define DHT_TIMEOUT_ERROR -2
#define DHT_CHECKSUM_ERROR -1
#define DHT_OKAY  0

mqtt_client *clientOut;

int humidity = 0;
int temperature = 0;
int Ftemperature = 0;

int DHT_PIN = 18;


const char *MQTT_TAG = "MQTT_SAMPLE";
const char *on_sgnl="on";
const char *off_sgnl="off";
char *temp_pub;
char *hum_pub;
static EventGroupHandle_t wifi_event_group;
const static int CONNECTED_BIT = BIT0;

/*
*
*DHT code
*
*/
void errorHandle(int response)
{
	switch(response)
	{
		case DHT_TIMEOUT_ERROR :
			printf("DHT Sensor Timeout!\n");
			break;
		case DHT_CHECKSUM_ERROR:
			printf("CheckSum error!\n");
			break;
		case DHT_OKAY:
			printf("All ok\n");
			break;
		default :
			printf("Unknown Error\n");
			break;
	}
}

void sendStart()
{
	//Send start signal from ESP32 to DHT device
	gpio_set_direction(DHT_PIN, GPIO_MODE_OUTPUT);
	//keep it low for at least 18ms
	gpio_set_level(DHT_PIN,0);
	ets_delay_us(22000);
	//pull it high and wait for DHT to pull it low for 20-40 us
	gpio_set_level(DHT_PIN,1);
	ets_delay_us(20);
	gpio_set_direction(DHT_PIN, GPIO_MODE_INPUT);
}

int getData()
{
	int counter = 0;
	uint8_t bits[5];
	uint8_t byteCounter = 0;
	uint8_t cnt = 7;
	memset(bits,0,5);

	sendStart();

	//Wait for a response from the DHT11 device for 20-40 us

	counter = 0;
	while (gpio_get_level(DHT_PIN)==1)
	{
		if(counter > 30)
		{
			return DHT_TIMEOUT_ERROR;
		}
		counter = counter + 1;
		ets_delay_us(1);
	}

	//DHT has pulled the line low and will keep the line low for 80 us and then high for 80us

	counter = 0;
	while(gpio_get_level(DHT_PIN)==0)
	{
		if(counter > 80)
		{
			return DHT_TIMEOUT_ERROR;
		}
		counter = counter + 1;
		ets_delay_us(1);
	}

	counter = 0;
	while(gpio_get_level(DHT_PIN)==1)
	{
		if(counter > 80)
		{
			return DHT_TIMEOUT_ERROR;
		}
		counter = counter + 1;
		ets_delay_us(1);
	}

	//Now output data from the DHT11 is 40 bits

	for(int i = 0; i < 40; i++)
	{
		counter = 0;
		//The 50 us low signal preceeding data bit
		while(gpio_get_level(DHT_PIN)==0)
		{
			if (counter > 55)
			{
			return DHT_TIMEOUT_ERROR;
			}
			counter = counter + 1;
			ets_delay_us(1);
		}

		//Read the data bit 70us means its 1 and 26-30 us means its low
		counter = 0;
		while(gpio_get_level(DHT_PIN)==1)
		{
			if (counter > 75)
			{
			return DHT_TIMEOUT_ERROR;
			}
			counter = counter + 1;
			ets_delay_us(1);
		}

		//Chk if the bit is high, if yes then set it in the bits array
		if (counter > 40)
		{
			bits[byteCounter] |= (1 << cnt);
		}

		//The byte is finished
		if (cnt == 0)
		{
			cnt = 7;
			byteCounter = byteCounter +1;
		}
		//decrement the bit position
		else
		{
			cnt-=1;
		}
	}
	float temp_c=bits[2];
	float temp_f=(temp_c * 1.8 + 32);
	humidity = bits[0];
	temperature = bits[2];
	Ftemperature = (int)temp_f;
	uint8_t sum = bits[0] + bits[2];

	if (bits[4] != sum)
	{
		return DHT_CHECKSUM_ERROR;
	}

	return DHT_OKAY;
}

void printDHT(void* pvParam)
{
    while(1)
	{
        //mqtt_client *client = malloc(sizeof(mqtt_client));
        printf("Temperature reading %d\n",temperature);
        printf("F temperature is %d\n", Ftemperature);
        printf("Humidity reading %d\n",humidity);
        mqtt_publish(clientOut, "/temp", temperature, sizeof(temperature), 0, 0);
        //mqtt_publish(client, "/humidity", humidity, sizeof(humidity), 0, 0);
        vTaskDelay(3000 / portTICK_RATE_MS);
	}
}


/*
*
*MQTT Code
*
*/
void connected_cb(void *self, void *params)
{
    mqtt_client *client = (mqtt_client *)self;
    //mqtt_subscribe(client, "/temp_dummy", 0);
    //mqtt_subscribe(client, "/humidity_dummy", 0);
    //mqtt_subscribe(client, "/water_condition", 0);
    mqtt_subscribe(client, "/water_level", 0);
}
void disconnected_cb(void *self, void *params)
{

}
void reconnect_cb(void *self, void *params)
{

}
void subscribe_cb(void *self, void *params)
{
    ESP_LOGI(MQTT_TAG, "[APP] Subscribe ok");
    mqtt_client *client = (mqtt_client *)self;
    //xTaskCreate(&DHT, "DHT", 1024, NULL, 2, NULL);
    //  xTaskCreate(&printDHT, "printDHT", 4024, NULL, 2, NULL);
    //mqtt_publish(client, "lights", "abcde", 5, 0, 0);
}
void openNow(void* pvParam)
{
    int value=4;
    while(value)
    {
        value-=1;
        gpio_set_level(25,1);
        //mqtt_publish(client, "/temp", temp_pub, sizeof(temp_pub), 0, 0);
        //mqtt_publish(client, "/humidity", hum_pub, sizeof(hum_pub), 0, 0);
        vTaskDelay(portTICK_PERIOD_MS/1000);
    }
    gpio_set_level(25,0);
    while(1){
        vTaskDelay(portTICK_PERIOD_MS/100);
    }
}

void publish_cb(void *self, void *params)
{

}
void data_cb(void *self, void *params)
{
    mqtt_client *client = (mqtt_client *)self;
    mqtt_event_data_t *event_data = (mqtt_event_data_t *)params;

    char *topic = malloc(event_data->topic_length + 1);
    if(event_data->data_offset == 0) {
        memcpy(topic, event_data->topic, event_data->topic_length);
        topic[event_data->topic_length] = 0;
        ESP_LOGI(MQTT_TAG, "[APP] Publish topic: %s", topic);
    }

    char *data = malloc(event_data->data_length + 1);
    memcpy(data, event_data->data, event_data->data_length);
    data[event_data->data_length] = 0;
    //ESP_LOGI(MQTT_TAG, "[APP] Publish data[%d/%d bytes] : %s",event_data->data_length+event_d     ata->data_offset,event_data->data_total_length, data);
    if(strcmp(topic,"/water_level")==0) {
        ESP_LOGI(MQTT_TAG, "%s Publish data[%d/%d bytes] : %s",topic,event_data->data_length + event_data->data_offset,event_data->data_total_length, data);
        xTaskCreate(&openNow, "openNow", 512, NULL, 2, NULL);
    }
    char buffer[10];
    itoa(temperature,buffer,10);
    if(strcmp(topic,"/temp_dummy")==0) mqtt_publish(client, "/temp", data, sizeof(data), 0, 0);
    else printf("Temperature reading %d %d\n",buffer,sizeof(buffer));
    itoa(humidity,buffer,10);
    if(strcmp(topic,"/humidity_dummy")==0) mqtt_publish(client, "/humidity", data, sizeof(data), 0,0);
    else mqtt_publish(client, "/humidity", buffer, sizeof(buffer), 0, 0);
    //if(strcmp(topic,"/water_condition")==0) ESP_LOGI(MQTT_TAG, "%s Publish data[%d/%d bytes] : %s",topic,event_data->data_length + event_data->data_offset,event_data->data_total_length, data);
    free(data);
    free(topic);

}

mqtt_settings settings = {
    .host = "m20.cloudmqtt.com",
#if defined(CONFIG_MQTT_SECURITY_ON)
    .port = 24917, // encrypted
#else
    .port = 14917, // unencrypted
#endif
    .client_id = "diwali_mqtt_lights",
    .username = "dfxukbgb",
    .password = "lq_IHyYetOBV",
    .clean_session = 0,
    .keepalive = 120,
    .lwt_topic = "/lwt",
    .lwt_msg = "offline",
    .lwt_qos = 0,
    .lwt_retain = 0,
    .connected_cb = connected_cb,
    .disconnected_cb = disconnected_cb,
//    .connect_cb = reconnect_cb,
    .subscribe_cb = subscribe_cb,
    .publish_cb = publish_cb,
    .data_cb = data_cb
};



static esp_err_t wifi_event_handler(void *ctx, system_event_t *event)
{
    switch(event->event_id) {
        case SYSTEM_EVENT_STA_START:
            esp_wifi_connect();
            break;
        case SYSTEM_EVENT_STA_GOT_IP:
            xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
            //vTaskDelay(portTICK_PERIOD_MS/5000);
            //clientOut=malloc(sizeof(mqtt_client));
            clientOut=mqtt_start(&settings);
            //xTaskCreate(&DHT, "DHT", 4024, NULL, 2, NULL);
            //xTaskCreate(&printDHT, "printDHT", 4048, NULL, 2, NULL);
            //init app here
            break;
        case SYSTEM_EVENT_STA_DISCONNECTED:
            /* This is a workaround as ESP32 WiFi libs don't currently
               auto-reassociate. */
            esp_wifi_connect();
            mqtt_stop();
            xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
            break;
        default:
            break;
    }
    return ESP_OK;
}

static void wifi_conn_init(void)
{
    tcpip_adapter_init();
    wifi_event_group = xEventGroupCreate();
    ESP_ERROR_CHECK(esp_event_loop_init(wifi_event_handler, NULL));
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_wifi_set_storage(WIFI_STORAGE_RAM));
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "batman22",
            .password = "wsvbatman",
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
    ESP_LOGI(MQTT_TAG, "start the WIFI SSID:[%s] password:[%s]", CONFIG_WIFI_SSID, "******");
    ESP_ERROR_CHECK(esp_wifi_start());
}

void app_main()
{
    temp_pub=malloc(10);
    hum_pub=malloc(10);
    gpio_pad_select_gpio(25);
    gpio_pad_select_gpio(18);
    gpio_set_direction(25, GPIO_MODE_OUTPUT);
    ESP_LOGI(MQTT_TAG, "[APP] Startup..");
    ESP_LOGI(MQTT_TAG, "[APP] Free memory: %d bytes", system_get_free_heap_size());
    ESP_LOGI(MQTT_TAG, "[APP] SDK version: %s, Build time: %s", system_get_sdk_version(), BUID_TIME);

    int resp=getData();
    if(resp!=DHT_OKAY)
    {
        errorHandle(resp);
    }
    vTaskDelay(portTICK_PERIOD_MS/1000);
    nvs_flash_init();
    wifi_conn_init();
    //while(1){       vTaskDelay(portTICK_PERIOD_MS/100);    }
}
