#include <stdio.h>
#include <windows.h>
#include "database.h"
#include "menu.h"

int main(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    UserSession session = {0}; // Инициализация сессии

    // Показать меню аутентификации
    show_auth_menu(&session);

    data_base db;
    init_system(&db, 10);

    // Загрузка данных пользователя
    if (session.is_authenticated) {
        char user_data_file[100];
        get_user_data_filename(session.username, user_data_file);
        load_from_file(&db, user_data_file);
    }

    show_main_menu(&db, session.username);
    
    // Сохранение данных при выходе
    if (session.is_authenticated) {
        char user_data_file[100];
        get_user_data_filename(session.username, user_data_file);
        save_to_file(&db, user_data_file);
    }
    
    free_system(&db);
    
    return 0;
}