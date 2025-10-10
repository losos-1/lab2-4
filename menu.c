#include "menu.h"
#include <stdio.h>
#include <windows.h>

// Очистка буфера ввода
void clear_input_buffer() {
    while (getchar() != '\n');
}

// Показать все записи
void handle_show_all(struct data_base* db) {
    printf("Вся база заказов:\n");
    display_items(db);
}

// Добавление новой записи
void handle_add_record(struct data_base* db) {
    technical_maintenance new_record;
    
    // Присвоение ID
    new_record.id = db->size + 1;
    int cnt_date = 0;
    while (cnt_date == 0)
    {   
        char new_date[11];
        printf("│ Введите дату в формате хх.хх.хххх:");
        scanf("%10s", new_date);
        if (validate_date(new_date) == 0) {
            printf("│ Ошибка ввода! Неверный формат даты.\n");
            continue;
        } else {
            strcpy(new_record.date, new_date);  // Копируем в структуру
            cnt_date++;
        }
    }
    clear_input_buffer();
    printf("│ Введите тип работы:");
    fgets(new_record.type_work, sizeof(new_record.type_work), stdin);
    new_record.type_work[strcspn(new_record.type_work, "\n")] = 0;
    
    // Автоматическое определение цены
    autoprice(&new_record);
    int cnt_mileage = 0;
    while (cnt_mileage == 0)
    {
        int mileage;
        printf("│ Введите пробег: ");
        scanf("%d", &mileage);
        if (mileage > 0){
            new_record.mileage = mileage;
            cnt_mileage++;
        }
        else{
            printf("│ Ошибка ввода!\n");
        }
    }

    clear_input_buffer();
    
    // Добавление записи в базу
    add_item(db, new_record);
    printf("│ Запись успешно добавлена!\n\n");
    
}

// Удаление записи
void handle_delete_record(struct data_base* db) {
    int cnt_selection4 = 0;
    while (cnt_selection4 == 0) {   
        int confirmation = 0;
        printf("│ Введите номер заказа, который хотите удалить\n");
        printf("│ Если хотите отменить операцию введите 0(ноль): ");
        int index_selection4;
        scanf("%d", &index_selection4);
        
        if (index_selection4 >= 1 && index_selection4 <= db->size) {
            printf("│ Вы уверены? (да-1, нет-0): ");
            scanf("%d", &confirmation);
            if (confirmation == 1) {
                printf("│ Хорошо! Заявка номер %d удалена\n", index_selection4);
                delete_item(db, index_selection4 - 1); 
                cnt_selection4++;
            } else if (confirmation == 0) {
                printf("│ Операция прекращена!\n");
                cnt_selection4++;
            } else {
                printf("│ Неверный ввод!\n");
            }
        } else if (index_selection4 == 0) {
            printf("│ Операция отменена!\n");
            cnt_selection4++;
        } else {
            printf("│ Такой записи не существует!\n");
        }
    }
}
// Редактирование записи 
void handle_edit_record(struct data_base* db) {
    printf("│ Введите номер заказа, который хотите изменить\n");
    printf("│ Если хотите отменить операцию введите 0(ноль): ");
    int index_selection3;
    scanf("%d", &index_selection3);
    
    if (index_selection3 == 0) {
        printf("│ Операция отменена!\n");
        return;
    }
    
    if (index_selection3 > 0 && index_selection3 <= db->size) {
        int actual_index = index_selection3 - 1;
        int selection_edit_record;
        printf("│ что вы хотите изменить?\n");
        printf("│ 1 - тип работы\n");
        printf("│ 2 - стоимость\n");
        printf("│ 3 - дату\n");
        printf("│ 4 - пробег\n");
        printf("│ выберите(1-4):");
        scanf("%d",&selection_edit_record);
        clear_input_buffer();
        switch(selection_edit_record){
            case 1:
                printf("│ Введите новый тип работы:");
                char new_type_work[100];
                fgets(new_type_work, sizeof(new_type_work), stdin);
                new_type_work[strcspn(new_type_work, "\n")] = 0;

                technical_maintenance updated = db->records[actual_index]; // копия масива
                strncpy(updated.type_work, new_type_work, sizeof(updated.type_work) - 1);
                modify_item(db, actual_index, updated);
                printf("│ Тип работы изменен!\n");
                break;
            
            case 2:
                printf("│ Введите новою стоимость:");
                float new_price;
                scanf("%f",&new_price);
                technical_maintenance updated_price = db -> records[actual_index];
                updated_price.price = new_price;
                modify_item(db,actual_index,updated_price);
                printf("│ Стоимость изменена!");
                break;
            case 3:
                printf("│ Введите новую дату:");
                char new_date[11];
                scanf("%10s",&new_date);
                
                technical_maintenance updated_date = db -> records[actual_index];
                strncpy(updated_date.date,new_date, sizeof(updated_date.date)-1);
                modify_item(db,actual_index,updated_date);
                printf("│ Дата изменена!");
                break;
            case 4:
                printf("│ Введите новый пробег:");
                int new_mileage;
                scanf("%d",&new_mileage);
                technical_maintenance updated_mileage = db -> records[actual_index];
                updated_mileage.mileage = new_mileage;
                modify_item(db,actual_index,updated_mileage);
                printf("│ Пробег изменен!\n");
                break;
            default:
                printf("│ Неверный ввод!");
        }
        

    } else {
        printf("│ Такого номера не существует!\n");
    }
}
// Главное меню
void show_main_menu(struct data_base* db) {
    int cnt = 0;
    while (cnt == 0) {
        printf("╔═════════════════════════════════════════╗\n");
        printf("║  Система управления базой данных автo   ║\n");
        printf("╠═════════════════════════════════════════╣\n");
        printf("║ Текущее количество записей в базе:%-4d     ║\n",db->size);
        printf("║ Главное меню:                           ║\n");
        printf("║ 1. Показать все заказы                  ║\n");
        printf("║ 2. Добавить новый заказ                 ║\n");
        printf("║ 3. Редактировать детали заказа          ║\n");
        printf("║ 4. Удалить заказ                        ║\n");
        printf("║ 5. сохранить изменение                  ║\n");
        printf("║ 6. Выход                                ║\n");
        printf("╚═════════════════════════════════════════╝\n\n");
        printf(" Выберите пункт меню (1-5):            \n");
        int selection;
        scanf("%d", &selection);
        clear_input_buffer();
        
        switch (selection) {
            case 1:
                handle_show_all(db);
                break;
            case 2:
                handle_add_record(db);
                break;
            case 3:
                handle_edit_record(db);
                break;
            case 4:
                handle_delete_record(db);
                break;
            case 5:
                 save_to_file(db, "auto_service.dat");
                break;
            case 6:
                printf("GGWP!\n");
                cnt++;
                break;
            default:
                printf("Ошибка! Неверный ввод\n");
                printf("Выберите пункт меню (1-5): ");
                break;
        }
    }
}
void handle_show_all(struct data_base* db) {
    if (db->size == 0) {
        printf("База данных пуста.\n");
        return;
    }
    printf("Вся база заказов:\n");
    display_items(db);
}