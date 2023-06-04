
#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include <inttypes.h>
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"

#include "esp_log.h"
#include "mqtt_client.h"
#include "esp_tls.h"
#include "esp_ota_ops.h"
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/rtc_io.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp32/rom/rtc.h"
#include "esp32/rom/crc.h"
#include "sys/time.h"
#include "sdkconfig.h"
#include <time.h>

#define TAG_rtc "RTC"

#if 1 /* GLobal varible */

    /* Data MQTT CLOUD */
    int led = 0;
	int alarmLed = 0;
	int startHour = 0;
	int startMinute = 0;
	int endHour = 0;
	int endMinute = 0;

    unsigned char glFlagTimer = 0;
    unsigned char datachange = 0;

    /* test alarm*/
    unsigned char first_Initialization = 1;
    unsigned char test = 1;
    unsigned char suspend = 0;
    TaskHandle_t xHandle;
    
    /* Time for alarm */
	struct timeAlarm {
		unsigned char Hours;
		unsigned char Minutes;
    };
    struct timeAlarm timeNow;
    time_t now;
    struct tm timeinfo;

    // Data upload 

    /* timer loop 10ms */
    
	unsigned char statusLed = 0;
	unsigned char statusAlarm = 0;
	unsigned char statusWifi = 0;
    char wifi;
    #define onLed 1
    #define offLed 0
    
#endif

#if 1
    /* setting RTC */
    void settupRTC(void);

	/* Set power Pump */
	void setPowerLed(unsigned char mode);
		
	/* Check data */
	void queryData();
		
	/* Alarm Pump */
	void setTimeRTC(int hour, int minute);
	void getTime();
	unsigned char alarmRTC(int hour, int minute);

    static void mqtt_app_start(void);
    void publishMQTT(char *topic,char *data[]);

    void updateCloudMQTT(void);
    void clearData();

    void operate();

    void setup(void);
#endif


static const char *TAG = "MQTTS_EXAMPLE";
static esp_mqtt_client_handle_t client;
#if CONFIG_BROKER_CERTIFICATE_OVERRIDDEN == 1
static const uint8_t mqtt_eclipseprojects_io_pem_start[]  = "-----BEGIN CERTIFICATE-----\n" CONFIG_BROKER_CERTIFICATE_OVERRIDE "\n-----END CERTIFICATE-----";
#else
extern const uint8_t mqtt_eclipseprojects_io_pem_start[]   asm("_binary_mqtt_eclipseprojects_io_pem_start");
#endif
extern const uint8_t mqtt_eclipseprojects_io_pem_end[]   asm("_binary_mqtt_eclipseprojects_io_pem_end");
//
// Note: this function is for testing purposes only publishing part of the active partition
//       (to be checked against the original binary)
//

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{

    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            msg_id = esp_mqtt_client_subscribe(client, "led", 2);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "alarmLed", 2);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "startHour", 2);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "startMinute", 2);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "endHour", 2);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "endMinute", 2);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

            msg_id = esp_mqtt_client_subscribe(client, "statusWifi", 1);
            ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
           

            //msg_id = esp_mqtt_client_unsubscribe(client, "/topic/qos1");
            //ESP_LOGI(TAG, "sent unsubscribe successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            //msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
            //ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");

            char *data = strndup(event->data, event->data_len);
			//printf("Gia tri cua message: %s\n", data);

            if (strncmp(event->topic, "led", event->topic_len) == 0) {
                led = atoi(data);
                datachange = datachange + 1;
                printf("led = %d \r\n",led);
                
            }
            if (strncmp(event->topic, "alarmLed", event->topic_len) == 0) {
                alarmLed = atoi(data);
                datachange = datachange + 1;
                printf("alarmLed = %d \r\n",alarmLed);
                
            }
            if (strncmp(event->topic, "startHour", event->topic_len) == 0) {
                startHour = atoi(data);
                datachange = datachange + 1;
                printf("startHour = %d \r\n",startHour);
               
            }
            if (strncmp(event->topic, "startMinute", event->topic_len) == 0) {
                startMinute = atoi(data);
                datachange = datachange + 1;
                printf("startMinute = %d \r\n",startMinute);
                
            }
            if (strncmp(event->topic, "endHour", event->topic_len) == 0) {
                endHour = atoi(data);
                datachange = datachange + 1;
                printf("endHour = %d \r\n",endHour);
               
            }
            if (strncmp(event->topic, "endMinute", event->topic_len) == 0) {
                endMinute = atoi(data);
                datachange = datachange + 1;
                printf("endMinute = %d \r\n",endMinute);
                
            }
            free(data);
            
             break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            if (event->error_handle->error_type == MQTT_ERROR_TYPE_ESP_TLS) {
                ESP_LOGI(TAG, "Last error code reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
                ESP_LOGI(TAG, "Last tls stack error number: 0x%x", event->error_handle->esp_tls_stack_err);
            } else if (event->error_handle->error_type == MQTT_ERROR_TYPE_CONNECTION_REFUSED) {
                ESP_LOGI(TAG, "Connection refused error: 0x%x", event->error_handle->connect_return_code);
            } else {
                ESP_LOGW(TAG, "Unknown error type: 0x%x", event->error_handle->error_type);
            }
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_app_start(void)
{
    const esp_mqtt_client_config_t mqtt_cfg = {
        .uri = CONFIG_BROKER_URI,
        .cert_pem = (const char *)mqtt_eclipseprojects_io_pem_start,
        .username = "hivemq.webclient.1680854521781",
        .password = "92<kXaAGtul>bH;Y1Q0!",
        .event_handle = mqtt_event_handler,
    };

    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void publishMQTT(char *topic,char *data[])
{
    //int esp_mqtt_client_publish(esp_mqtt_client_handle_t client, const char *topic, const char *data, int len, int qos, int retain)
    //default qos = 1 and retain = 0;
    esp_mqtt_client_publish(client, topic, data, strlen(data), 2, 0);
}


/*void timer_callback(void *param)
{
    glFlagtimer10ms = 1;
}*/

///////////////////////////////////////////////////////////////
/* Check Data */
void queryData()
{
	/* off led */
	if(led == 0 && alarmLed == 0){
		setPowerLed(offLed);
		statusLed = 0;
		statusAlarm = 0;
		//glResetTimerFlag = 0;
		//clearData();
  	}
	else if(led == 0 && alarmLed == 1 && statusAlarm == 0){
    	setPowerLed(onLed);
		statusLed = 1;
		glFlagTimer = 0;

		// Start alarm
		setTimeRTC(startHour,startMinute);
	  	statusAlarm = 1;
        printf(" startAlarm \r\n ");	
	}
	else if(led == 1 && alarmLed == 0 && statusAlarm == 0){
		setPowerLed(onLed);
		statusLed = 1;
		//glResetTimerFlag = 0;
		
		// Limit time Led = 20 minute 
		//setLimitedTime();
		//setTime(sHour,sMinute);
		//statusAlarm = 1;
		//glLimitedTimeFlag = 1;
	}
	/*else if(led == 1 && alarmLed == 1 && statusAlarm == 0){
		setPowerLed(onLed);
		statusLed = 1;
		//glResetTimerFlag = 0;

		// Start alarm
		setTimeRTC(startHour,startMinute);
		statusAlarm = 1;
	}*/
		
}
////////////////////////////////////////////////////////////

/* set Power Pump */
void setPowerLed(unsigned char mode)
{
	gpio_set_level(23,mode);
}

void controlRTC(void *pvParameter)
{
    while(1){
        if(statusAlarm == 1){
            //printf("alarming\r\n");
            statusAlarm = alarmRTC(endHour,endMinute);

            if(statusAlarm == 0){
                // turn Off Led
                setPowerLed(offLed);
                statusLed = 0;
                //glLimitedTimeFlag = 0;
                
                /* update led state */
                updateCloudMQTT();
                clearData();
                printf(" finnish alarm ");
                suspend = 1;
		    }
		    // continute alarm-------->
        }
        else{
            printf("No alarming");
        }
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

#if 0
void setupRTC(void)
{
    esp_err_t ret;
	struct tm default_time = {
        .tm_year = 2023 - 1900,
        .tm_mon = 3, // January
        .tm_mday = 10,
        .tm_hour = ....,
        .tm_min = .....,
        .tm_sec = 0
    };
    time_t default_time_secs = mktime(&default_time);
    struct timeval tv = {
        .tv_sec = default_time_secs,
        .tv_usec = 0
    };
    printf("Setting default time to %ld\n", default_time_secs);
    settimeofday(&tv, NULL);

    // Initialize the RTC GPIO
    printf("Initializing RTC GPIO\n");
    ret = rtc_gpio_init(GPIO_NUM_26);
    if (ret != ESP_OK) {
        printf("Error initializing RTC GPIO: %d\n", ret);
        //return;
    }
    ret = rtc_gpio_set_direction(GPIO_NUM_26, RTC_GPIO_MODE_INPUT_ONLY);
    if (ret != ESP_OK) {
        printf("Error setting RTC GPIO direction: %d\n", ret);
        //return;
    }
}
#endif
////////////////////////////////////////////////////////////
/* Alarm Pump */
void setTimeRTC(int hour, int minute)
{   
    esp_err_t ret;
	struct tm default_time = {
        .tm_year = 2023 - 1900,
        .tm_mon = 3, // January
        .tm_mday = 10,
        .tm_hour = hour,
        .tm_min = minute,
        .tm_sec = 0
    };
    time_t default_time_secs = mktime(&default_time);
    struct timeval tv = {
        .tv_sec = default_time_secs,
        .tv_usec = 0
    };
    //printf("Setting default time to %ld\n", default_time_secs);
    settimeofday(&tv, NULL);

    // Initialize the RTC GPIO
    //printf("Initializing RTC GPIO\n");
    ret = rtc_gpio_init(GPIO_NUM_26);
    if (ret != ESP_OK) {
        printf("Error initializing RTC GPIO: %d\n", ret);
        //return;
    }
    ret = rtc_gpio_set_direction(GPIO_NUM_26, RTC_GPIO_MODE_INPUT_ONLY);
    if (ret != ESP_OK) {
        printf("Error setting RTC GPIO direction: %d\n", ret);
        //return;
    }
    printf("set alarm success \r\n");
}

void getTime()
{
    time(&now);
    localtime_r(&now, &timeinfo);
	// get time now 
	timeNow.Hours = timeinfo.tm_hour;
	timeNow.Minutes = timeinfo.tm_min;
}

unsigned char alarmRTC(int hour, int minute)
{
	unsigned char flag;

	//Read Time from RTC
	getTime();
    printf("%d %d \r\n", timeNow.Hours, timeNow.Minutes); 

	if(timeNow.Hours == hour){
		if(timeNow.Minutes < minute){
			flag = 1;     // continnue alarm
		}
		else{
			flag = 0;     // stop alarm
		}
	}
	else{
		flag = 1;         // continnue alarm
	}
	return flag;
}
////////////////////////////////////////////////////////////


void mainTask(void *pvParameter)
{
    while(1){
        /* update connect Esp to Wifi state */
        if(statusWifi == 0){
            publishMQTT("statusWifi","0");
            statusWifi = 1;
        }
        else{
            publishMQTT("statusWifi","1");
            statusWifi = 0;
        }

        /* incoming data from MQTT cloud */
        if(datachange == 6){

            /* Process Data */
            queryData();

            if(statusAlarm == 1 && glFlagTimer == 0){
                /* start task alarm */
                if(first_Initialization == 1){
                    xTaskCreate(&controlRTC, "controlRTC", 2048, NULL, 4, &xHandle);
                    printf("create task control \r\n");
                    first_Initialization = 0;
                }
                else{
                    vTaskResume(xHandle);
                    printf("Resume\r\n");
                }
                glFlagTimer = 1;
            }
            datachange = 0;
            updateCloudMQTT();
        }
        
        /* suspend when no need RTC */
        if(suspend == 1){
            suspend = 0;
            vTaskSuspend(xHandle);
            printf("suspend done\r\n");
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void updateCloudMQTT(void)
{
    /* Updata state led */
    if(statusLed == 1){
        publishMQTT("statusLed","1");
    }
    else{
        publishMQTT("statusLed","0");
    }
}

void clearData(void)
{
    led = 0;
    alarmLed = 0;
    startHour = 0;
    startMinute = 0;
    endHour = 0;
    endMinute = 0;

    datachange = 0;
}

void setup(void)
{
    /* Set gpio output for control relay */
    gpio_pad_select_gpio(23);
    gpio_set_direction(23,GPIO_MODE_OUTPUT);
   
    clearData();
}

void operate(void)
{
    // if(test == 1){
        
    //     led = 0;
    //     alarmLed = 1;
    //     startHour = 7;
    //     startMinute = 0;
    //     endHour = 7;
    //     endMinute = 1;
    //     queryData();
    //     printf("process Data");
    //     xTaskCreate(&controlRTC, "controlRTC", 2048, NULL, 5, &xHandle);
    //     test = 0;
    // }

    // if(suspend == 1){
    //     vTaskSuspend(xHandle);
    //     suspend = 0;
    //     printf("suspent task");
    // }
    
    //updateDatatoMQTT();
    //esp_mqtt_client_publish(client, "statusWifi", "12", 2, 1, 0);
    
    //publishMQTT("statusWifi","1");
    //publishMQTT("data","1 2 3 4");
}


void app_main(void)
{
    setup();
    
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_TCP", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_SSL", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    
    mqtt_app_start();
    
    xTaskCreate(&mainTask, "mainTask", 2048, NULL, 5, NULL);

    
}


