#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "individueel.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"

static int credits = 0;
static item shop_items[MAX_AMOUNT_ITEMS];
static item inventory_items[MAX_AMOUNT_ITEMS];

static int index_item = 0;
int size_shop;
int size_inventory;

static enum catagory status;
static void create_items(void);


//in betwween method for the setup for the player
void setup_player()
{
    create_items();
}

//creates all the items
static void create_items()
{
    item item_0 = {"Candy", 25};
    item item_1 = {"Bear, Tiny", 50};
    item item_2 = {"Football", 100};
    item item_3 = {"Bear, Huge", 150};
    item item_4 = {"Toy B&A", 400};
    item item_5 = {"Toy gun", 500};
    item item_6 = {"HeadPhones", 600};
    item item_7 = {"MP3-Player", 800};
    
    
    shop_items[0] = item_0;
    shop_items[1] = item_1;
    shop_items[2] = item_2;
    shop_items[3] = item_3;
    shop_items[4] = item_4;
    shop_items[5] = item_5;
    shop_items[6] = item_6;
    shop_items[7] = item_7;

    size_shop = 8;
    size_inventory = 0;
}

//buys the selected item, not used for now. Still in defelopment
void buy_item()
{
    item selected_item = shop_items[index_item];

    if(credits >= selected_item.price){
        credits -= selected_item.price;
         for(int i = index_item; i < size_shop-1; i++)
        {
            shop_items[i] = shop_items[i + 1];
        }
       
        inventory_items[size_inventory] = selected_item;
        size_inventory++;
        size_shop--;
    }
}

//raises the index
void raise_index()
{
    index_item++;
    switch(status)
    {
        case SHOP:
            if(index_item >= size_shop)
            {
                index_item = 0;
            }
            break;

        case INVENTORY:
            if(index_item >= size_inventory)
            {
                index_item = 0;
            }
            break;
        
        default:
            ESP_LOGW("RAISE_INDEX", "VALUE CAN NOT BE RAISED, STATUS UNKNOWN!!!");
            break;
    }
}

//lowers the index
void lower_index()
{
    index_item--;
    
    if(index_item < 0)
    {
        switch(status)
        {
            case SHOP:
                index_item = size_shop;
                break;

            case INVENTORY:
                index_item = size_inventory;
             break;
        
            default:
                ESP_LOGW("RAISE_INDEX", "VALUE CAN NOT BE lowered, STATUS UNKNOWN!!!");
                break;
         }
    }
}

//gets the show size
int get_shop_items_size()
{
    return size_shop;
}

//gets the inventory size
int get_inventory_items_size()
{
    return size_inventory;
}

//set the current status, like shop or inventory
void set_status(enum catagory new_status)
{
    status = new_status;
}

//gets the current status, like in the shop or in the inventory
enum catagory get_status(){
    return status;
}

//gives an amount of credits
void give_credits(int value)
{
    credits += value;
    ESP_LOGI("CREDITS", "credits: %d", credits);
}

//gets the current amount of creadits
int get_credits()
{
    return credits;
}

// returns all the shop items
item *get_all_shop_items(){
    return shop_items;
}
