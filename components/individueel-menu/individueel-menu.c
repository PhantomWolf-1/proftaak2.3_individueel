#include "individueel-menu.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdio.h>
#include <string.h>

#define MENUTAG "menu"


static i2c_lcd1602_info_t *_lcd_info;
static menu_t *_menu;
static unsigned int custom_chars[MAX_CUSTOM_CHARACTERS];

void create_custom_characters(void);
void menu_spin_slot(void);
enum outcome calculate_win(unsigned int numbers[]);


i2c_lcd1602_info_t * lcd_init(void){
    i2c_port_t i2c_num = I2C_MASTER_NUM;
    uint8_t address = CONFIG_LCD1602_I2C_ADDRESS;

    // Set up the SMBus
    smbus_info_t * smbus_info = smbus_malloc();
    smbus_init(smbus_info, i2c_num, address);
    smbus_set_timeout(smbus_info, 1000 / portTICK_RATE_MS);

    // Set up the LCD1602 device with backlight off
    i2c_lcd1602_info_t * lcd_info = i2c_lcd1602_malloc();
    i2c_lcd1602_init(lcd_info, smbus_info, true, LCD_NUM_ROWS, LCD_NUM_COLUMNS, LCD_NUM_VIS_COLUMNS);

    // turn off backlight
    ESP_LOGI(MENUTAG, "backlight off");
    i2c_lcd1602_set_backlight(lcd_info, false);

    // turn on backlight
    ESP_LOGI(MENUTAG, "backlight on");
    i2c_lcd1602_set_backlight(lcd_info, true);

    // turn on cursor 
    ESP_LOGI(MENUTAG, "cursor on");
    i2c_lcd1602_set_cursor(lcd_info, true);

    return lcd_info;
}

menu_t *menu_create_menu(void){

    menu_t *menuPointer = malloc(sizeof(menu_t));

    // Temporary array of menu items to copy from
    menu_item_t menuItems[MAX_MENU_ITEMS] = {
        {MENU_MAIN_ID_0, {MENU_SLOTS_ID_0, MENU_MAIN_ID_2, MENU_MAIN_ID_1}, {"MAIN MENU", "Slots"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_MAIN_ID_1, {MENU_SHOP_ID_0, MENU_MAIN_ID_0, MENU_MAIN_ID_2}, {"MAIN MENU", "Shop"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_MAIN_ID_2, {MENU_INVENTORY_ID_0, MENU_MAIN_ID_1, MENU_MAIN_ID_0}, {"MAIN MENU", "Inventory"}, {NULL, NULL, NULL}, NULL, NULL},

        {MENU_SLOTS_ID_0, {MENU_SLOTS_ID_0, MENU_SLOTS_ID_1, MENU_SLOTS_ID_1}, {"SLOTS", "Spin"}, {menu_spin_slot, NULL, NULL}, NULL, NULL},
        {MENU_SLOTS_ID_1, {MENU_MAIN_ID_0, MENU_SLOTS_ID_0, MENU_SLOTS_ID_0}, {"SLOTS", "Back"}, {NULL, NULL, NULL}, NULL, NULL}, 

        {MENU_SHOP_ID_0, {MENU_SHOP_ID_2, MENU_SHOP_ID_1, MENU_SHOP_ID_1}, {"SHOP", "Items"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_SHOP_ID_1, {MENU_MAIN_ID_1, MENU_SHOP_ID_0, MENU_SHOP_ID_0}, {"SHOP", "Back"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_SHOP_ID_2, {MENU_SHOP_ID_2, MENU_SHOP_ID_2, MENU_SHOP_ID_2}, {"ITEMS", " ", "", ""}, {NULL, NULL, NULL}, NULL, NULL},

        {MENU_INVENTORY_ID_0, {MENU_INVENTORY_ID_2, MENU_INVENTORY_ID_1, MENU_INVENTORY_ID_1}, {"INVENTORY", "Items"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_INVENTORY_ID_1, {MENU_MAIN_ID_2, MENU_INVENTORY_ID_0, MENU_INVENTORY_ID_0}, {"INVENTORY", "Back"}, {NULL, NULL, NULL}, NULL, NULL},
        {MENU_INVENTORY_ID_2, {MENU_INVENTORY_ID_2, MENU_INVENTORY_ID_2, MENU_INVENTORY_ID_2}, {"ITEMS", " ", "", ""}, {NULL, NULL, NULL}, NULL, NULL},
    };
    
    _lcd_info = lcd_init();
    
    // If allocation is succesful, set all values
    if(menuPointer != NULL)
    {
        // Initialize menu with values
        menuPointer->lcd_info = _lcd_info;
        menuPointer->menuItems = calloc(MAX_MENU_ITEMS, sizeof(menu_item_t));
        memcpy(menuPointer->menuItems, menuItems, MAX_MENU_ITEMS * sizeof(menu_item_t));
        menuPointer->currentMenuItemId = MENU_MAIN_ID_0;

        _menu = menuPointer;

        ESP_LOGD(MENUTAG, "malloc menu_t %p", menuPointer);
    }
    else 
    {
        ESP_LOGD(MENUTAG, "malloc menu_t failed");
    }

    create_custom_characters();

    return menuPointer;
}


// Frees memory used by menu pointer
void menu_free_menu(menu_t *menu)
{
    free(menu->lcd_info);
    menu->lcd_info = NULL;
    free(menu->menuItems);
    menu->menuItems = NULL;
    free(menu);
    menu = NULL;
}


// Displays menu all lines from menu item on lcd 
void menu_display_menu_item(menu_t *menu, int menuItemId)
{
    i2c_lcd1602_clear(menu->lcd_info);

    for (size_t line = 0; line < MAX_LCD_LINES; line++) {
        const char* menuText = menu->menuItems[menuItemId].menuText[line];
        int textPosition = 0;
        i2c_lcd1602_move_cursor(menu->lcd_info, textPosition, line);
        i2c_lcd1602_write_string(menu->lcd_info, menuText);
    }
}


void menu_write_scroll_menu_item(i2c_lcd1602_info_t *lcd_info, char* text, int line)
{
    int textPosition = 0;
    i2c_lcd1602_move_cursor(lcd_info, textPosition, line);
    i2c_lcd1602_write_string(lcd_info, text);
}


void menu_display_welcome_message(menu_t *menu)
{
    i2c_lcd1602_set_cursor(menu->lcd_info, false);
    i2c_lcd1602_move_cursor(menu->lcd_info, 6, 1);

    i2c_lcd1602_write_string(menu->lcd_info, "Welcome");
    i2c_lcd1602_move_cursor(menu->lcd_info, 8, 2);
    i2c_lcd1602_write_string(menu->lcd_info, "User");

    vTaskDelay(2500 / portTICK_RATE_MS);
    i2c_lcd1602_clear(menu->lcd_info);
}


void menu_display_scroll_menu(menu_t *menu)
{
    i2c_lcd1602_clear(menu->lcd_info);
    
    // Gets title of scroll menu
    char *menuText = menu->menuItems[menu->currentMenuItemId].menuText[0];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 0);

    // Get item before currentMenuItem
    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_LEFT]].menuText[1];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 1);

    // Get currentMenuItem
    menuText = menu->menuItems[menu->currentMenuItemId].menuText[1];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 2);

    // Get item after currentMenuItem
    menuText = menu->menuItems[menu->menuItems[menu->currentMenuItemId].otherIds[MENU_KEY_RIGHT]].menuText[1];
    menu_write_scroll_menu_item(menu->lcd_info, menuText, 3);

    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(menu->lcd_info, 17, 2);
    i2c_lcd1602_write_string(menu->lcd_info, cursor);
}

static unsigned int get_random_number(){
    int number = (rand() % (MAX_CUSTOM_CHARACTERS - LOW_CUSTOM_CHARACTERS + 1)) + LOW_CUSTOM_CHARACTERS;
    return number;
}

void menu_spin_slot()
{
    unsigned int spins[MAX_SLOTS];
    for(int i = 0; i < MAX_SLOTS; i++)
    {
        for(int j = 0; j < SPIN_ROLLS; j++)
        {
            i2c_lcd1602_move_cursor(_menu->lcd_info, (i * 2) + 10, 0);
            unsigned int random_number = get_random_number();
            ESP_LOGI("RANDOM_NUMBER", "%d", random_number);
            vTaskDelay(TIME_BETWEEN_ROLLS / portTICK_RATE_MS);
            i2c_lcd1602_write_custom_char(_menu->lcd_info, custom_chars[random_number]);
            spins[i] = random_number;
        }
        vTaskDelay(TIME_BETWEEN_SPINS / portTICK_RATE_MS);
    }

    enum outcome result;
    result = calculate_win(spins);
    
    switch(result)
    {
        case LOSE:
            
            break;

        case TINY_WIN: 
            
            break;

        case BIG_WIN:
            
            break;

        case ERROR: 
            
            break;

        default:
            ESP_LOGW("CALCULATE_SPIN", "WRONG OUTCOME!!!");
            break;
    }

}


enum outcome calculate_win(unsigned int numbers[])
{
    int arr[1000],i,j,count=0, min;
    for(i = 0; i < MAX_SLOTS; i++)
    {
        min = i;
        for(j = i + 1; j < MAX_SLOTS; j++)
        {
            if(arr[min]>arr[j])
            {
                min = j;    
            }
        }
        {
            int temp = arr[min];
            arr[min] = arr[i];
            arr[i] = temp;
        }
    }

    for(i = 1; i < MAX_SLOTS; i++)
    {
        if(arr[i]==arr[i-1])
        {
            count++;
            while(arr[i]==arr[i-1]) i++;
        }
    }

    switch(count)
    {
        case 0:
            return LOSE;
            break;

        case 1: 
            return LOSE;
            break;

        case 2:
            return TINY_WIN;
            break;

        case 3: 
            return BIG_WIN;
            break;

        default:
            return ERROR;
            break;
    }
}

void menu_handle_key_event(menu_t *menu, int key)
{
    // If key press leads to the same ID as the currentMenuItemId
    // do not switch to a new menu item, instead call the onKey event
    if(menu->menuItems[menu->currentMenuItemId].otherIds[key] == menu->currentMenuItemId){
        // Call the onKey event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnKeyEvent[key] != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnKeyEvent[key])();
        }
    } else {
        // Call the onMenuExit event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnMenuExitEvent != NULL) {
        (*menu->menuItems[menu->currentMenuItemId].fpOnMenuExitEvent)();
        }

        menu->currentMenuItemId = menu->menuItems[menu->currentMenuItemId].otherIds[key];

        // Display menu on LCD
        if(strcmp(menu->menuItems[menu->currentMenuItemId].menuText[1], " ") == 0) {
            menu_display_menu_item(menu, menu->currentMenuItemId);
        } else {
            menu_display_scroll_menu(menu);
        }

        // Call the onMenuEntry event function if there is one
        if(menu->menuItems[menu->currentMenuItemId].fpOnMenuEntryEvent != NULL) {
            (*menu->menuItems[menu->currentMenuItemId].fpOnMenuEntryEvent)();
        }
    }
}


void create_custom_characters(void)
{
    uint8_t diamond[8] = {
    0B00000,
    0B00000,
    0B00100,
    0B01110,
    0B11111,
    0B01110,
    0B00100,
    0B00000
    };

    uint8_t cross[8] = {
    0B00000,
    0B00000,
    0B10001,
    0B01010,
    0B00100,
    0B01010,
    0B10001,
    0B00000
    };

    uint8_t circle[8] = {
    0B00000,
    0B00000,
    0B01110,
    0B10001,
    0B10001,
    0B10001,
    0B01110,
    0B00000
    };

    uint8_t square[8] = {
    0B00000,
    0B00000,
    0B11111,
    0B11111,
    0B11111,
    0B11111,
    0B11111,
    0B00000
    };

    uint8_t plus[8] = {
    0B00000,
    0B00000,
    0B00100,
    0B00100,
    0B11111,
    0B00100,
    0B00100,
    0B00000
    };

    uint8_t grail[8] = {
    0B00000,
    0B00000,
    0B11111,
    0B11111,
    0B01110,
    0B00100,
    0B01110,
    0B00000
    };

    custom_chars[0] = I2C_LCD1602_INDEX_CUSTOM_0;
    custom_chars[1] = I2C_LCD1602_INDEX_CUSTOM_1;
    custom_chars[2] = I2C_LCD1602_INDEX_CUSTOM_2;
    custom_chars[3] = I2C_LCD1602_INDEX_CUSTOM_3;
    custom_chars[4] = I2C_LCD1602_INDEX_CUSTOM_4;
    custom_chars[5] = I2C_LCD1602_INDEX_CUSTOM_5;


    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_0, diamond);
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_1, cross);
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_2, circle);
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_3, square);
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_4, plus);
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_5, grail);
}
