#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include "driver/gpio.h"
#include <esp_log.h>
#include "dht.h"
#include <string.h>
#include "ssd1306.h"
#include "font_latin_8x8.h"
#include "esp_flash.h"
#include "esp_wifi.h"
#include "nvs_flash.h"
#include <esp_err.h>
#include "mqtt_client.h"

#define DHT_GPIO 23
const TickType_t xDelay = 2000 / portTICK_PERIOD_MS;
static char *TAG = "Temp";
static char temp[50] = {0};
ssd1306_config_t dev_cfg = I2C_SSD1306_128x64_CONFIG_DEFAULT;
ssd1306_handle_t dev_hdl;
esp_mqtt_client_handle_t client;
bool mqtt_connected = false;  // Track MQTT connection status

void wifi_event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data);
void handle_error(esp_err_t);
void display_init();
void draw(char *text);
esp_err_t wifi_init(const char *ssid, const char *pass);
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
void mqtt_app_start(const char *broker_uri);
i2c_master_bus_handle_t i2c_init();

void app_main(void)
{
    char buf[50];
    float temp_val = 0, humid = 0;
    esp_err_t result;
    
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);
    
    display_init();
    draw("Initializing...");
    vTaskDelay(2000 / portTICK_PERIOD_MS); 
    wifi_init("Optilink","aryan0011");
    mqtt_app_start("mqtt://broker.mqtt.cool");
    
    while(1) {
        result = dht_read_float_data(DHT_TYPE_DHT11, DHT_GPIO, &humid, &temp_val);
        
        if (result == ESP_OK) {
            snprintf(buf, sizeof(buf), "T:%.1fC H:%.1f%%", temp_val, humid);
            draw(buf);

            if (mqtt_connected && client) {
                int msg_id = esp_mqtt_client_publish(client, "/dht/aryan", buf, 0, 1, 0);
                if (msg_id != -1) {
                    ESP_LOGI(TAG, "Published successfully, msg_id=%d, data=%s", msg_id, buf);
                } else {
                    ESP_LOGW(TAG, "Failed to publish message");
                }
            } else {
                ESP_LOGW(TAG, "MQTT not connected, skipping publish");
            }
        } else {
            ESP_LOGW(TAG, "Failed to read DHT11 sensor");
            draw("Sensor Error!");
        }
        
        vTaskDelay(xDelay);
    }
}

void handle_error(esp_err_t err) {
    switch (err) {
        case ESP_FAIL:
            ESP_LOGE(TAG, "DHT sensor read failed");
            break;
        case ESP_OK:
            ESP_LOGI(TAG, "DHT sensor initialized successfully");
            break;
        default:
            ESP_LOGE(TAG, "Unknown error: %s", esp_err_to_name(err));
            break;
    }
}

i2c_master_bus_handle_t i2c_init() {
    i2c_master_bus_handle_t bus;
    i2c_master_bus_config_t i2c_mst_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .scl_io_num = 22,
        .sda_io_num = 21,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };
    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_mst_config, &bus));
    return bus;
}

void display_init() {
    i2c_master_bus_handle_t bus = i2c_init();
    ESP_ERROR_CHECK(ssd1306_init(bus, &dev_cfg, &dev_hdl));
}

void draw(char *text) {
    if(text && strcmp(text, temp) != 0) {
        ssd1306_clear_display(dev_hdl, false);
        ssd1306_display_text(dev_hdl, 0, text, false); 
        
        strncpy(temp, text, sizeof(temp)-1);
        temp[sizeof(temp)-1] = '\0';
    }
}

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "WiFi disconnected, retrying...");
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
    }
}

esp_err_t wifi_init(const char *ssid, const char *pass) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));

    wifi_config_t wifi_config = {};
    strncpy((char *)wifi_config.sta.ssid, ssid, sizeof(wifi_config.sta.ssid));
    strncpy((char *)wifi_config.sta.password, pass, sizeof(wifi_config.sta.password));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi init done, connecting to %s...", ssid);
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT Connected");
            mqtt_connected = true;
            esp_mqtt_client_subscribe(event->client, "/topic/qos0", 0);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT Disconnected");
            mqtt_connected = false;
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT Subscribed, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "Topic: %.*s, Data: %.*s",
                     event->topic_len, event->topic,
                     event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT Error");
            mqtt_connected = false;
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
}

void mqtt_app_start(const char *broker_uri)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = broker_uri,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}