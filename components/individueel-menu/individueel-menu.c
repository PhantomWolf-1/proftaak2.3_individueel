#include "individueel-menu.h"
#include "individueel.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define MENUTAG "menu"


static i2c_lcd1602_info_t *_lcd_info;
static menu_t *_menu;
unsigned int custom_chars[MAX_CUSTOM_CHARACTERS];

int item_index = 0;

void create_custom_characters(void);
void menu_spin_slot(void);
enum outcome calculate_win(unsigned int numbers[]);
int calculate(unsigned int numbers[]);
void menu_display_credits(void);
void diplay_items(void);
void next_item(void);
void previous_item(void);
void enter_item_list_shop(void);
void enter_item_list_inventory(void);

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
        {MENU_MAIN_ID_0, {MENU_SLOTS_ID_0, MENU_MAIN_ID_2, MENU_MAIN_ID_1}, {"MAIN MENU", "Slots"}, {NULL, NULL, NULL}, menu_display_credits, NULL},
        {MENU_MAIN_ID_1, {MENU_SHOP_ID_0, MENU_MAIN_ID_0, MENU_MAIN_ID_2}, {"MAIN MENU", "Shop"}, {NULL, NULL, NULL}, menu_display_credits, NULL},
        {MENU_MAIN_ID_2, {MENU_INVENTORY_ID_0, MENU_MAIN_ID_1, MENU_MAIN_ID_0}, {"MAIN MENU", "Inventory"}, {NULL, NULL, NULL}, menu_display_credits, NULL},

        {MENU_SLOTS_ID_0, {MENU_SLOTS_ID_0, MENU_SLOTS_ID_1, MENU_SLOTS_ID_1}, {"SLOTS", "Spin"}, {menu_spin_slot, NULL, NULL}, menu_display_credits, NULL},
        {MENU_SLOTS_ID_1, {MENU_MAIN_ID_0, MENU_SLOTS_ID_0, MENU_SLOTS_ID_0}, {"SLOTS", "Back"}, {NULL, NULL, NULL}, menu_display_credits, NULL}, 

        {MENU_SHOP_ID_0, {MENU_SHOP_ID_2, MENU_SHOP_ID_1, MENU_SHOP_ID_1}, {"SHOP", "Items"}, {NULL, NULL, NULL}, menu_display_credits, NULL},
        {MENU_SHOP_ID_1, {MENU_MAIN_ID_1, MENU_SHOP_ID_0, MENU_SHOP_ID_0}, {"SHOP", "Back"}, {NULL, NULL, NULL}, menu_display_credits, NULL},
        {MENU_SHOP_ID_2, {MENU_SHOP_ID_2, MENU_SHOP_ID_2, MENU_SHOP_ID_2}, {"ITEMS", " ", "", ""}, {NULL, previous_item, next_item}, enter_item_list_shop, NULL},

        {MENU_INVENTORY_ID_0, {MENU_INVENTORY_ID_2, MENU_INVENTORY_ID_1, MENU_INVENTORY_ID_1}, {"INVENTORY", "Items"}, {NULL, NULL, NULL}, menu_display_credits, NULL},
        {MENU_INVENTORY_ID_1, {MENU_MAIN_ID_2, MENU_INVENTORY_ID_0, MENU_INVENTORY_ID_0}, {"INVENTORY", "Back"}, {NULL, NULL, NULL}, menu_display_credits, NULL},
        {MENU_INVENTORY_ID_2, {MENU_INVENTORY_ID_2, MENU_INVENTORY_ID_2, MENU_INVENTORY_ID_2}, {"ITEMS", " ", "", ""}, {NULL, NULL, NULL}, enter_item_list_inventory, NULL},
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

//writes a scroll menu item on the lcd
void menu_write_scroll_menu_item(i2c_lcd1602_info_t *lcd_info, char* text, int line)
{
    int textPosition = 0;
    i2c_lcd1602_move_cursor(lcd_info, textPosition, line);
    i2c_lcd1602_write_string(lcd_info, text);
}

//displays a welcom message
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

//displays scroll menu 
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
    i2c_lcd1602_move_cursor(menu->lcd_info, 11, 2);
    i2c_lcd1602_write_string(menu->lcd_info, cursor);
}

//displays the imtes for the shop and inventory, currently works only on the shop, by mechanism not implemented yet
void display_items()
{
    i2c_lcd1602_clear(_menu->lcd_info);
    char * menuText;
    int previous_item_index, next_item_index;
    item *all_items;

    // Display scroll menu title
    menuText = "SHOP";
    menu_write_scroll_menu_item(_menu->lcd_info, menuText, 0);
    switch(get_status())
    {
        case SHOP:
            all_items = get_all_shop_items();
            if(item_index + 1 >= get_shop_items_size + 1)
            {
                item_index = 0;
            }
            else if(item_index - 1 < -1)
            {
                item_index = get_shop_items_size;
            }
            previous_item_index = item_index - 1 < 0? get_shop_items_size() : item_index - 1;
            menuText = all_items[previous_item_index].name;
            menu_write_scroll_menu_item(_lcd_info, menuText, 1);

            menuText = all_items[item_index].name;
            menu_write_scroll_menu_item(_lcd_info, menuText, 2);

            next_item_index = item_index + 1 > get_shop_items_size()? 0 : item_index + 1;
            menuText = all_items[next_item_index].name;
            menu_write_scroll_menu_item(_lcd_info, menuText, 3);  
            break;
        
        case INVENTORY:
            break;

        default:
            ESP_LOGI("STATUS", "NO STATUS!!!");
            break;
    }

    // Display cursor
    const char *cursor = "<";
    i2c_lcd1602_move_cursor(_lcd_info, 11, 2);
    i2c_lcd1602_write_string(_lcd_info, cursor);
}

//sets the index plus one for the scroll machanism
void next_item()
{
    item_index++;
    display_items();
}

//sets the index minus one for the scroll machanism
void previous_item()
{
    item_index--;
    display_items();
}

//gets a random number
static unsigned int get_random_number()
{
    int number = (rand() % (MAX_CUSTOM_CHARACTERS - LOW_CUSTOM_CHARACTERS + 1)) + LOW_CUSTOM_CHARACTERS;
    return number;
}


//lets the spin machine roll
void menu_spin_slot()
{
    srand(time(0));
    unsigned int spins[MAX_SLOTS];
    //gets the numbers that are rolled 
    for(int i = 0; i < MAX_SLOTS; i++)
    {
        for(int j = 0; j < SPIN_ROLLS; j++)
        {
            i2c_lcd1602_move_cursor(_menu->lcd_info, (i * 2) + 13, 2);
            unsigned int random_number = get_random_number();
            vTaskDelay(TIME_BETWEEN_ROLLS / portTICK_RATE_MS);
            i2c_lcd1602_write_custom_char(_menu->lcd_info, custom_chars[random_number]);
            spins[i] = random_number;
        }
        vTaskDelay(TIME_BETWEEN_SPINS / portTICK_RATE_MS);
    }

    //calculates the amount of duplicates in the roll
    int result = calculate(spins);
    switch(result)
    {
        case 1: 
            give_credits(50);
            break;

        case 2:
            give_credits(100);
            break;

        default:
            ESP_LOGI("CALCULATE_SPIN", "no price!!!");
            break;
    }
    //refreshes the credit count
    menu_display_credits();
}

//calculates the duplicates
int calculate(unsigned int numbers[])
{
    int counter = 0;
    for (int i = 0; i < MAX_SLOTS; i++) {
        for(int j = i+1; j < MAX_SLOTS; j++)
        {
            if(numbers[i] == numbers[j]){
                counter++;
            }
        }
    }
    return counter;
}

//sets a number to a char[]
static char * toString(int number) {
    int length = snprintf(NULL, 0, "%d", number + 1);
    char *str = malloc(length + 1);
    snprintf(str, length + 1, "%d", number);
    return str;
}

//displays the credit
void menu_display_credits(void){
    i2c_lcd1602_move_cursor(_menu->lcd_info, 15, 0);
    i2c_lcd1602_write_string(_menu->lcd_info, toString(get_credits()));
}

//displays the correct things for the item list for the shop
void enter_item_list_shop(){
    set_status(SHOP);
    display_items();
    menu_display_credits();
}

//displays the correct things for the item list for the inventory
void enter_item_list_inventory(){
    set_status(INVENTORY);
    display_items();
    menu_display_credits();
}

//handles the key event
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

//creates the custom characters
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

    //define them in the open slots for xustom charcters
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_0, diamond);
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_1, cross);
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_2, circle);
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_3, square);
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_4, plus);
    i2c_lcd1602_define_char(_menu->lcd_info, I2C_LCD1602_INDEX_CUSTOM_5, grail);
}

