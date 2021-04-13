#ifndef INDIV_H
#define INDIV_H

#define MAX_ITEM_NAME 15
#define MAX_AMOUNT_ITEMS 8

enum catagory{SHOP, INVENTORY};

typedef struct{
    char name[MAX_ITEM_NAME];
    int price;
} item;


void setup_player(void);
void buy_item(void);

void raise_index();
void lower_index();
int get_shop_items_size();
int get_inventory_items_size();
void set_status(enum catagory new_status);

void give_credits(int value);
int get_credits();
enum catagory get_status();

char **get_all_shop_item_names();
item *get_all_shop_items();

#endif