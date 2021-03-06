#ifndef INDIV_MENU
#define INDIV_MENU

#include "i2c-lcd1602.h"

//init
#undef USE_STDIN

#define I2C_MASTER_NUM           I2C_NUM_0
#define I2C_MASTER_TX_BUF_LEN    0                     // disabled
#define I2C_MASTER_RX_BUF_LEN    0                     // disabled
#define I2C_MASTER_FREQ_HZ       100000
#define I2C_MASTER_SDA_IO        CONFIG_I2C_MASTER_SDA
#define I2C_MASTER_SCL_IO        CONFIG_I2C_MASTER_SCL
#define LCD_NUM_ROWS			 4
#define LCD_NUM_COLUMNS			 40
#define LCD_NUM_VIS_COLUMNS		 20

// Menu size settings
#define MAX_MENU_ITEMS 22
#define MAX_MENU_KEYS 3
#define MAX_LCD_LINES 4
#define LOW_CUSTOM_CHARACTERS 0
#define MAX_CUSTOM_CHARACTERS 6

#define MAX_SLOTS 3
#define SPIN_ROLLS 5
#define TIME_BETWEEN_ROLLS 500
#define TIME_BETWEEN_SPINS 800

// Possible key presses
#define MENU_KEY_OK 0
#define MENU_KEY_LEFT 1
#define MENU_KEY_RIGHT 2

// Menu screen IDs
#define MENU_MAIN_ID_0 0
#define MENU_MAIN_ID_1 1
#define MENU_MAIN_ID_2 2

#define MENU_SLOTS_ID_0 3
#define MENU_SLOTS_ID_1 4

#define MENU_SHOP_ID_0 5
#define MENU_SHOP_ID_1 6
#define MENU_SHOP_ID_2 7

#define MENU_INVENTORY_ID_0 8
#define MENU_INVENTORY_ID_1 9
#define MENU_INVENTORY_ID_2 10

enum outcome{BIG_WIN, TINY_WIN, LOSE, ERROR};

typedef struct {
    unsigned int id;
    unsigned int otherIds[MAX_MENU_KEYS];
    char *menuText[MAX_LCD_LINES];
    void (*fpOnKeyEvent[MAX_MENU_KEYS])(void);
    void (*fpOnMenuEntryEvent)(void);
    void (*fpOnMenuExitEvent)(void);
} menu_item_t;

typedef struct {
    i2c_lcd1602_info_t *lcd_info;
    menu_item_t *menuItems;
    unsigned int currentMenuItemId;
} menu_t;


i2c_lcd1602_info_t * lcd_init(void);
menu_t *menu_create_menu(void);
void menu_free_menu(menu_t *menu);
void menu_display_welcome_message(menu_t *menu);
void menu_display_scroll_menu(menu_t *menu);
void menu_display_menu_item(menu_t *menu, int menuItemId);
void menu_handle_key_event(menu_t *menu, int key);

void create_custom_characters(void);
void display_items(void);
#endif