# Makefile для сборки системы автосервиса с ASF парсером

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = 
TARGET = auto_service.exe
SOURCES = main_updated.c auto.c logic.c database.c menu.c \
          asf_parser.c asf_serializer.c data_adapter.c database_new.c
HEADERS = database.h menu.h asf_parser.h data_adapter.h database_new.h
OBJECTS = $(SOURCES:.c=.o)

# Основная цель
all: $(TARGET)

# Сборка исполняемого файла
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)

# Компиляция отдельных файлов
%.o: %.c $(HEADERS)
	$(CC) $(CFLAGS) -c $< -o $@

# Тест парсера
test_parser: asf_parser.o asf_serializer.o data_adapter.o
	$(CC) $(CFLAGS) -o test_parser test_parser.c asf_parser.o asf_serializer.o data_adapter.o $(LDFLAGS)
	./test_parser

# Очистка
clean:
	del /Q *.o *.exe test_parser 2>nul || true
	rm -f *.o $(TARGET) test_parser 2>/dev/null || true

# Запуск
run: $(TARGET)
	./$(TARGET)

# Отладка
debug: CFLAGS += -DDEBUG -O0
debug: clean $(TARGET)

.PHONY: all clean run debug test_parser