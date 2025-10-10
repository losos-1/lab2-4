#ifndef MENU_H
#define MENU_H

#include "database.h"

// Прототипы функций меню
void show_main_menu(struct data_base* db);
void handle_show_all(struct data_base* db);
void handle_add_record(struct data_base* db);
void handle_edit_record(struct data_base* db);
void handle_delete_record(struct data_base* db);
void clear_input_buffer();


#endif