#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "database.h"

// Простая хеш-функция на основе XOR и циклического сдвига
unsigned int simple_hash(const char* password) {
    unsigned int hash = 0;
    for (int i = 0; password[i] != '\0'; ++i) {
        hash = (hash << 3) | (hash >> 29); // Циклический сдвиг
        hash ^= password[i]; // XOR с текущим символом
    }
    return hash;
}

// Генерация имени файла данных для пользователя
void get_user_data_filename(const char* username, char* filename) {
    sprintf(filename, "data_%s.dat", username);
}

// Проверка существования пользователя
int user_exists(const char* username) {
    FILE* file = fopen("users.dat", "rb");
    if (!file) {
        return 0;
    }
    
    User existing_user;
    while (fread(&existing_user, sizeof(User), 1, file) == 1) {
        if (strcmp(existing_user.username, username) == 0) {
            fclose(file);
            return 1;
        }
    }
    
    fclose(file);
    return 0;
}

// Сохранение пользователя с хешированным паролем
void save_user(const User* new_user) {
    FILE* file = fopen("users.dat", "ab");
    if (!file) {
        printf("Ошибка создания файла пользователей!\n");
        return;
    }

    fwrite(new_user, sizeof(User), 1, file);
    fclose(file);
}

// Проверка учетных данных
int validate_credentials(const char* username, const char* password) {
    FILE* file = fopen("users.dat", "rb");
    if (!file) {
        printf("│ Файл users.dat не найден!\n");
        return 0;
    }
    
    User existing_user;
    int found = 0;
    
    while (fread(&existing_user, sizeof(User), 1, file) == 1) {
        if (strcmp(existing_user.username, username) == 0) {
            found = 1;
            unsigned int input_hash = simple_hash(password);
            
            fclose(file);
            return (input_hash == existing_user.password_hash);
        }
    }
    
    fclose(file);
    
    if (!found) {
        printf("│ Пользователь '%s' не найден!\n", username);
    }
    
    return 0;
}

// Функция входа пользователя
int login_user(UserSession* session) {
    char username[51];
    char password[51];
    
    printf("\n╔═════════════════════════════════════════╗\n");
    printf("║                  ВХОД                   ║\n");
    printf("╚═════════════════════════════════════════╝\n");
    
    printf("│ Введите логин: ");
    if (scanf("%50s", username) != 1) {
        printf("│ Ошибка ввода логина!\n");
        while (getchar() != '\n');
        return 0;
    }
    
    printf("│ Введите пароль: ");
    if (scanf("%50s", password) != 1) {
        printf("│ Ошибка ввода пароля!\n");
        while (getchar() != '\n');
        return 0;
    }
    
    if (validate_credentials(username, password)) {
        strcpy(session->username, username);
        session->is_authenticated = 1;
        printf("│ Вход выполнен успешно!\n");
        printf("│ Добро пожаловать, %s!\n\n", username);
        return 1;
    } else {
        printf("│ Неверный логин или пароль!\n");
        return 0;
    }
}

// Функция регистрации пользователя
int register_user(UserSession* session) {
    User new_user;
    
    printf("\n╔═════════════════════════════════════════╗\n");
    printf("║              РЕГИСТРАЦИЯ                ║\n");
    printf("╚═════════════════════════════════════════╝\n");
    
    printf("│ Введите логин: ");
    if (scanf("%50s", new_user.username) != 1) {
        printf("│ Ошибка ввода логина!\n");
        while (getchar() != '\n');
        return 0;
    }
    
    if (user_exists(new_user.username)) {
        printf("│ Пользователь с таким логином уже существует!\n");
        return 0;
    }
    
    printf("│ Введите пароль: ");
    char password[51];
    if (scanf("%50s", password) != 1) {
        printf("│ Ошибка ввода пароля!\n");
        while (getchar() != '\n');
        return 0;
    }

    new_user.password_hash = simple_hash(password);
    
    save_user(&new_user);
    printf("│ Регистрация успешно завершена!\n");
    printf("│ Добро пожаловать, %s!\n\n", new_user.username);
    
    // Автоматический вход после регистрации
    strcpy(session->username, new_user.username);
    session->is_authenticated = 1;
    
    return 1;
}

// Меню аутентификации
void show_auth_menu(UserSession* session) {
    int authenticated = 0;
    
    while (!authenticated) {
        printf("╔═════════════════════════════════════════╗\n");
        printf("║    СИСТЕМА УПРАВЛЕНИЯ АВТОСЕРВИСОМ      ║\n");
        printf("╠═════════════════════════════════════════╣\n");
        printf("║ 1. Вход                                 ║\n");
        printf("║ 2. Регистрация                          ║\n");
        printf("║ 3. Выход                                ║\n");
        printf("╚═════════════════════════════════════════╝\n");
        
        int choice;
        printf(" Выберите действие (1-3): ");
        if (scanf("%d", &choice) != 1) {
            printf("│ Ошибка ввода!\n");
            while (getchar() != '\n');
            continue;
        }
        
        switch (choice) {
            case 1:
                authenticated = login_user(session);
                break;
            case 2:
                if (register_user(session)) {
                    authenticated = 1;
                }
                break;
            case 3:
                printf("│ До свидания!\n");
                exit(0);
                break;
            default:
                printf("│ Неверный выбор! Попробуйте снова.\n");
                break;
        }
        while (getchar() != '\n');
    }
}