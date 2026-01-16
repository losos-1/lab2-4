#include <stdio.h>
#include <windows.h>
#include "database.h"
#include "menu.h"
#include "database_new.h"

int main(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);
    
    // Тестируем парсер (можно убрать после отладки)
    printf("Запуск системы с поддержкой ASF формата...\n");
    test_asf_parser();
    
    UserSession session = {0};
    
    // Показать меню аутентификации
    show_auth_menu(&session);
    
    data_base db;
    init_system(&db, 10);
    
    // Загрузка данных пользователя (старый бинарный формат)
    if (session.is_authenticated) {
        char user_data_file[100];
        get_user_data_filename(session.username, user_data_file);
        
        // Пробуем загрузить из нового ASF формата
        char asf_filename[100];
        snprintf(asf_filename, sizeof(asf_filename), "data_%s.asf", session.username);
        
        FILE* asf_file = fopen(asf_filename, "r");
        if (asf_file) {
            // Файл ASF существует - загружаем из него
            fclose(asf_file);
            printf("Обнаружен файл в новом ASF формате, загружаем...\n");
            load_from_asf(&db, asf_filename);
        } else {
            // Пробуем загрузить из старого бинарного формата
            printf("Файл ASF не найден, пробуем загрузить из старого формата...\n");
            load_from_file(&db, user_data_file);
            
            // Если загрузили из старого формата, предлагаем сохранить в новый
            if (db.size > 0) {
                printf("\n⚠️  Обнаружены данные в старом формате.\n");
                printf("Хотите автоматически конвертировать в новый ASF формат? (1-Да/0-Нет): ");
                
                int choice;
                if (scanf("%d", &choice) == 1 && choice == 1) {
                    save_to_asf(&db, asf_filename, session.username);
                    printf("✅ Данные конвертированы в ASF формат!\n");
                }
                while (getchar() != '\n'); // Очистка буфера
            }
        }
    }
    
    // Запускаем главное меню
    show_main_menu(&db, session.username);
    
    // Сохранение данных при выходе
    if (session.is_authenticated) {
        char asf_filename[100];
        snprintf(asf_filename, sizeof(asf_filename), "data_%s.asf", session.username);
        
        printf("\nСохранение данных при выходе...\n");
        printf("1. Сохранить в новом ASF формате (рекомендуется)\n");
        printf("2. Сохранить в старом бинарном формате\n");
        printf("3. Сохранить в обоих форматах\n");
        printf("Выберите вариант (1-3): ");
        
        int choice;
        if (scanf("%d", &choice) == 1) {
            switch (choice) {
                case 1:
                    save_to_asf(&db, asf_filename, session.username);
                    break;
                case 2: {
                    char user_data_file[100];
                    get_user_data_filename(session.username, user_data_file);
                    save_to_file(&db, user_data_file);
                    break;
                }
                case 3:
                    save_to_asf(&db, asf_filename, session.username);
                    char user_data_file[100];
                    get_user_data_filename(session.username, user_data_file);
                    save_to_file(&db, user_data_file);
                    printf("✅ Данные сохранены в обоих форматах!\n");
                    break;
                default:
                    printf("❌ Неверный выбор, данные не сохранены!\n");
                    break;
            }
        }
    }
    
    free_system(&db);
    
    printf("\nСпасибо за использование системы!\n");
    return 0;
}