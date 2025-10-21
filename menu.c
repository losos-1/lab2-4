#include "menu.h"
#include <stdio.h>
#include <windows.h>

// Очистка буфера ввода
void clear_input_buffer() {
    while (getchar() != '\n');
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
        clear_input_buffer(); // Очистка буфера после scanf
        
        if (validate_date(new_date) == 0) {
            printf("│ Ошибка ввода! Неверный формат даты.\n");
            continue;
        } else {
            strcpy(new_record.date, new_date);  // Копируем в структуру
            cnt_date++;
        }
    }
    
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
        int result = scanf("%d", &mileage);
        clear_input_buffer(); // Всегда очищаем буфер после scanf
        
        if (result != 1) {
            printf("│ Ошибка ввода! Введите число.\n");
            continue;
        }
        
        if (mileage > 0){
            new_record.mileage = mileage;
            cnt_mileage++;
        }
        else{
            printf("│ Ошибка ввода! Пробег должен быть положительным.\n");
        }
    }
    
    // Добавление записи в базу
    add_item(db, new_record);
    printf("│ Запись успешно добавлена!\n\n");
}
//Удаление всей базы данных
void handle_clear_database(struct data_base* db) {
    int cnt_clear = 0;
    while (cnt_clear == 0) {   
        printf("│ ВНИМАНИЕ! Это удалит ВСЕ записи без возможности восстановления!\n");
        printf("│ Вы уверены? (1-Да/0-Нет): ");
        int confirmation;
        if (scanf("%d", &confirmation) == 1) {
            clear_input_buffer();
            if (confirmation == 1) {
                clear_database(db);
                printf("│ База данных полностью очищена!\n");
                cnt_clear++;
            } else if (confirmation == 0) {
                printf("│ Операция отменена!\n");
                cnt_clear++;
            } else {
                printf("│ Неверный ввод! Введите 1 или 0\n");
            }
        } else {
            clear_input_buffer();
            printf("│ Неверный ввод! Введите 1 или 0\n");
        }
    }
}
// Удаление записи
void handle_delete_record(struct data_base* db) {
    int cnt_selection4 = 0;
    while (cnt_selection4 == 0) {   
        int confirmation = 0;
        printf("│ Введите номер заказа, который хотите удалить\n");
        printf("│ Отмена оперции - 0(ноль): ");
        printf("│ Удаление всей базы данных - (-1): ");
        int index_selection4;
        scanf("%d", &index_selection4);
        clear_input_buffer(); // Очистка буфера
        
        if (index_selection4 >= 1 && index_selection4 <= db->size) {
            printf("│ Вы уверены? (да-1, нет-0): ");
            scanf("%d", &confirmation);
            clear_input_buffer(); // Очистка буфера
            
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
    clear_input_buffer(); // Очистка буфера
    
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

                technical_maintenance updated = db->records[actual_index];
                strncpy(updated.type_work, new_type_work, sizeof(updated.type_work) - 1);
                modify_item(db, actual_index, updated);
                printf("│ Тип работы изменен!\n");
                break;
            
            case 2:
                printf("│ Введите новую стоимость:");
                float new_price;
                int result = scanf("%f", &new_price);
                clear_input_buffer(); // Очистка буфера
                
                if (result == 1) {
                    technical_maintenance updated_price = db->records[actual_index];
                    updated_price.price = new_price;
                    modify_item(db, actual_index, updated_price);
                    printf("│ Стоимость изменена!\n");
                } else {
                    printf("│ Ошибка ввода стоимости!\n");
                }
                break;
                
            case 3:
                printf("│ Введите новую дату:");
                char new_date[11];
                int cnt_validate_date = 0;
                while (cnt_validate_date == 0)
                {
                    scanf("%10s", new_date);
                    clear_input_buffer(); // Очистка буфера
                    
                    if (validate_date(new_date) == 1) {
                        technical_maintenance updated_date = db->records[actual_index];
                        strncpy(updated_date.date, new_date, sizeof(updated_date.date)-1);
                        modify_item(db, actual_index, updated_date);
                        printf("│ Дата изменена!\n");
                        cnt_validate_date++;
                    } else {
                        printf("│ Неверный формат ввода!\n");
                        printf("│ Введите новую дату:"); 
                    }
                }
                break;
                
            case 4:
                printf("│ Введите новый пробег:");
                int new_mileage;
                int cnt_validate_mileage = 0;
                
                while (cnt_validate_mileage == 0) {
                    int result = scanf("%d", &new_mileage);
                    
                    if (result != 1) {
                        printf("│ Неверный ввод! Ожидается число.\n");
                        printf("│ Введите новый пробег:");
                        clear_input_buffer();
                        continue;
                    }

                    clear_input_buffer();
                    
                    if (validate_mileage(new_mileage) == 1) {
                        technical_maintenance updated_mileage = db->records[actual_index];
                        updated_mileage.mileage = new_mileage;
                        modify_item(db, actual_index, updated_mileage);
                        printf("│ Пробег изменен!\n");
                        cnt_validate_mileage++;
                    } else {
                        printf("│ Неверный ввод! Пробег не может быть отрицательным.\n");
                        printf("│ Введите новый пробег:");
                    }
                }
                break;
                
            default:
                printf("│ Неверный выбор!\n");
                break;
        } 
    } else {
        printf("│ Такого номера не существует!\n");
    }
} 
int get_int_input(const char* prompt, int min, int max) {
    int value;
    
    while (1) {
        if (prompt != NULL) {
            printf("%s", prompt);
        }
        
        if (scanf("%d", &value) == 1 && value >= min && value <= max) {
            return value;
        }
        while (getchar() != '\n');
        printf("Ошибка! Введите число от %d до %d\n", min, max);
    }
}
// Главное меню
void show_main_menu(struct data_base* db) {
    int cnt = 0;
    while (cnt == 0) {
        printf("╔═════════════════════════════════════════╗\n");
        printf("║  Система управления базой данных автo   ║\n");
        printf("╠═════════════════════════════════════════╣\n");
        printf("║ Текущее количество записей в базе:%-4d  ║\n",db->size);
        printf("║ Главное меню:                           ║\n");
        printf("║ 1. Показать все заказы                  ║\n");
        printf("║ 2. Добавить новый заказ                 ║\n");
        printf("║ 3. Редактировать детали заказа          ║\n");
        printf("║ 4. Удалить заказ                        ║\n");
        printf("║ 5. сохранить изменение                  ║\n");
        printf("║ 6. Загрузить данные                     ║\n");
        printf("║ 7. Очистить всю базу данных             ║\n");
        printf("║ 8. Выход                                ║\n");
        printf("╚═════════════════════════════════════════╝\n\n");
        
        int selection = get_int_input(" Выберите пункт меню (1-8): ", 1, 8);
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
                int cnt_case5 = 0;
                while (cnt_case5 == 0)
                {   
                    printf("│ Вы уверены что хотите сохранить данные? (1-Да/0-Нет): ");
                    int confirmation;
                    if (scanf("%d", &confirmation) == 1) {
                        clear_input_buffer();
                        if (confirmation == 1) {
                            save_to_file(db, "auto_service.dat");
                            printf("│ Данные успешно сохранены!\n");
                            cnt_case5++;
                        } else if (confirmation == 0) {
                            printf("│ Сохранение отменено.\n");
                            cnt_case5++;
                        } else {
                            printf("│ Неверный ввод! Введите 1 или 0\n");
                        }
                    } else {
                        clear_input_buffer();
                        printf("│ Неверный ввод! Введите 1 или 0\n");
                    }
                }
                break;
            case 6:
                printf("│ ВНИМАНИЕ: Текущие данные будут потеряны!\n");
                printf("│ Вы уверены что хотите загрузить данные из файла? (1-Да/0-Нет): ");
                int load_confirmation;
                if (scanf("%d", &load_confirmation) == 1) {
                    clear_input_buffer();
                    if (load_confirmation == 1) {
                        load_from_file(db, "auto_service.dat");
                        printf("│ Данные загружены!\n");
                    } else {
                        printf("│ Загрузка отменена.\n");
                    }
                } else {
                    clear_input_buffer();
                    printf("│ Неверный ввод! Введите 1 или 0\n");
                }
                break;
            case 7:
                handle_clear_database(db);
                break;
            case 8:
                printf("GGWP!\n");
                cnt++;
                break;
        }
    }
}
void handle_show_all(struct data_base* db) {
    if (db->size == 0) {
        printf("│ Нет записей для отображения.\n");
        return;
    }
    
    printf("╔════╦════════════╦══════════════════════════════╦══════════╦═════════════╗\n");
    printf("║ ID ║    Дата    ║         Тип работы           ║  Пробег  ║  Стоимость  ║\n");
    printf("╠════╬════════════╬══════════════════════════════╬══════════╬═════════════╣\n");
    
    for (int i = 0; i < db->size; i++) {
        printf("║ %-2d ║ %-10s ║ %-28s ║ %-8d ║ %-11.2f ║\n", 
               db->records[i].id, 
               db->records[i].date, 
               db->records[i].type_work, 
               db->records[i].mileage, 
               db->records[i].price);
    }
    
    printf("╚════╩════════════╩══════════════════════════════╩══════════╩═════════════╝\n");
}