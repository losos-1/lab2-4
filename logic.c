#include <stdio.h>
#include <windows.h>
#include "database.h"


void autoprice(technical_maintenance* record) {
    if (strcmp(record->type_work, "замена масла") == 0 || 
        strcmp(record->type_work, "Замена масла") == 0) {
        record->price = 2100;
    } else if (strcmp(record->type_work, "осмотр ТС") == 0 || 
               strcmp(record->type_work, "Осмотр ТС") == 0) {
        record->price = 999.99;
    } else if (strcmp(record->type_work, "замена фильтра") == 0 || 
               strcmp(record->type_work, "Замена фильтра") == 0) {
        record->price = 14999.90;
    } else if (strcmp(record->type_work, "покраска кузова") == 0 || 
               strcmp(record->type_work, "Покраска кузова") == 0) {
        record->price = 33500.90;
    } else if (strcmp(record->type_work, "ремонт двигателя") == 0 || 
               strcmp(record->type_work, "Ремонт двигателя") == 0) {
        record->price = 150000.90;
    } else if (strcmp(record->type_work, "полное ТО") == 0 || 
               strcmp(record->type_work, "Полное ТО") == 0) {
        record->price = 8999.90;
    } else {
        printf("│ Введите стоимость: ");
        scanf("%f", &record->price);
    }
}