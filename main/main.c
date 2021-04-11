#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_log.h"
#include "sdkconfig.h"
#include "esp32/rom/uart.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include "esp_event.h"
#include "esp_attr.h"
#include "esp_sleep.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include "esp_task_wdt.h"
#include "smbus.h"
#include "qwiic_twist.h"
#include "i2c-lcd1602.h"
#include "clock-sync.h"
#include "esp_wifi.h"

//components that we made
#include "lcd-menu.h"
#include "wifi-connect.h"

#define MAINTAG "main"
#define CLOCKTAG "clock"
#define APITAG "api"

i2c_port_t i2c_num;
qwiic_twist_t *qwiic_twist_rotary;
menu_t *menu;

void rotary_task(void *);
void clicked(void);
void pressed(void);
void onMove(int16_t);

void i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf;
    conf.mode = I2C_MODE_MASTER;
    conf.sda_io_num = I2C_MASTER_SDA_IO;
    conf.sda_pullup_en = GPIO_PULLUP_DISABLE;  // GY-2561 provides 10kΩ pullups
    conf.scl_io_num = I2C_MASTER_SCL_IO;
    conf.scl_pullup_en = GPIO_PULLUP_DISABLE;  // GY-2561 provides 10kΩ pullups
    conf.master.clk_speed = I2C_MASTER_FREQ_HZ;
    i2c_param_config(i2c_master_port, &conf);
    i2c_driver_install(i2c_master_port, conf.mode,
                       I2C_MASTER_RX_BUF_LEN,
                       I2C_MASTER_TX_BUF_LEN, 0);
}

static void component_init(void){
    //INIT rotary encoder, and the qwiic_twist_init will set the oher settings for the rotary encoder
    qwiic_twist_rotary = (qwiic_twist_t*)malloc(sizeof(*qwiic_twist_rotary));
    qwiic_twist_rotary->port = i2c_num;
    qwiic_twist_rotary->onButtonClicked = &clicked;
    qwiic_twist_rotary->onButtonPressed = &pressed;
    qwiic_twist_rotary->onMoved = &onMove;
    qwiic_twist_init(qwiic_twist_rotary);

}


void menu_task(void * pvParameter)
{
    menu = menu_create_menu();

    menu_display_welcome_message(menu);
    menu_display_scroll_menu(menu);

    qwiic_twist_start_task(qwiic_twist_rotary);


    while(1)
    {
        vTaskDelay(2500 / portTICK_RATE_MS);
    }

    menu_free_menu(menu);
    vTaskDelete(NULL);
}

char * toString(int number) {
    int length = snprintf(NULL, 0, "%d", number + 1);
    char *str = malloc(length + 1);
    snprintf(str, length + 1, "%d", number + 1);
    return str;
}

/*
 * This method handles the key event "OK" (onButtonClicked), this is necessary for navigating through the menu.
 */
void clicked(void){
    ESP_LOGI(MAINTAG, "clicked rotary encoder");
    menu_handle_key_event(menu, MENU_KEY_OK);
}

/*
 *  This method is not used. Its a placeholder method (onButtonPressed).
 */
void pressed(void){
    ESP_LOGI(MAINTAG, "pressed rotary encoder");
}

/*
 *  This method handles the key event turning left and right (onMoved). This is necessary for navigating through the menu, cause this is the scrolling event.
 */
void onMove(int16_t move_value){
    if(move_value > 0){
        menu_handle_key_event(menu, MENU_KEY_RIGHT);
    }
    else if(move_value < 0){
        menu_handle_key_event(menu, MENU_KEY_LEFT);
    }
}

void rotary_task(void * pvParameter)
{
    qwiic_twist_start_task(qwiic_twist_rotary);
    while(1){
        vTaskDelay(1000 / portTICK_RATE_MS);
    }

    vTaskDelete(NULL);
}




void app_main()
{
    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    /* This helper function configures Wi-Fi as selected in menuconfig. */
    ESP_ERROR_CHECK(wifi_connect());
    vTaskDelay(1000);

    //I^2C initialization + the I^2C port
    i2c_master_init();
    i2c_num = I2C_MASTER_NUM;

    //initialize the components
    component_init();

    xTaskCreate(&menu_task, "menu_task", 4096, NULL, 5, NULL);
}

