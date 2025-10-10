#include <stdio.h>
#include <windows.h>
#include "database.h"
#include "menu.h"

int main(void) {
    SetConsoleOutputCP(CP_UTF8);
    SetConsoleCP(CP_UTF8);

    data_base db;
    init_system(&db, 10); // создание БД с начальной емкостью 10
    
    load_from_file(&db, "auto_service.dat");

    show_main_menu(&db);
    
    save_to_file(&db, "auto_service.dat");
    // Освобождение памяти перед выходом
    free_system(&db);
    
    return 0;
}