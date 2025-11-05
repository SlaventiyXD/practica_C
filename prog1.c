#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>

#define MAX_PRODUCTS 10
#define MAX_PRODUCT_NAME 50
#define SALES_FILE "sales.txt"

// Структура для передачи данных в поток
typedef struct {
    int thread_id;
    char product_name[MAX_PRODUCT_NAME];
    int min_delay;
    int max_delay;
    int sales_count;
} thread_data_t;

// Мьютекс для синхронизации доступа к файлу
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Функция потока - имитация продаж
void* sales_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    printf("Поток %d начал продажи товара: %s\n", 
           data->thread_id, data->product_name);
    
    for (int i = 0; i < data->sales_count; i++) {
        // Генерируем случайную задержку
        int delay = data->min_delay + rand() % (data->max_delay - data->min_delay + 1);
        sleep(delay);
        
        // Блокируем мьютекс для записи в файл
        pthread_mutex_lock(&file_mutex);
        
        FILE* file = fopen(SALES_FILE, "a");
        if (file != NULL) {
            fprintf(file, "Поток %d: Продажа %d товара '%s' (задержка: %d сек)\n", 
                    data->thread_id, i + 1, data->product_name, delay);
            fclose(file);
            
            printf("Поток %d: Продажа %d товара '%s' (задержка: %d сек)\n", 
                   data->thread_id, i + 1, data->product_name, delay);
        } else {
            printf("Ошибка открытия файла!\n");
        }
        
        // Разблокируем мьютекс
        pthread_mutex_unlock(&file_mutex);
    }
    
    printf("Поток %d завершил продажи товара: %s\n", 
           data->thread_id, data->product_name);
    
    return NULL;
}

int main() {
    // Инициализация генератора случайных чисел
    srand(time(NULL));
    
    // Очищаем файл продаж при запуске
    FILE* file = fopen(SALES_FILE, "w");
    if (file != NULL) {
        fclose(file);
        printf("Файл продаж очищен\n");
    }
    
    // Данные для потоков (товары)
    thread_data_t thread_data[] = {
        {1, "Ноутбук", 2, 5, 2},
        {2, "Смартфон", 1, 3, 3},
        {3, "Наушники", 1, 4, 3},
        {4, "Планшет", 3, 6, 1}
    };
    
    int num_threads = sizeof(thread_data) / sizeof(thread_data[0]);
    pthread_t threads[num_threads];
    
    printf("Запуск %d потоков продаж...\n", num_threads);
    printf("========================================\n");
    
    // Создаем потоки
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, sales_thread, &thread_data[i]) != 0) {
            perror("Ошибка создания потока");
            return 1;
        }
    }
    
    // Ожидаем завершения всех потоков
    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Ошибка ожидания потока");
            return 1;
        }
    }
    
    printf("========================================\n");
    printf("Все продажи завершены. Результаты записаны в файл: %s\n", SALES_FILE);
    
    // Уничтожаем мьютекс
    pthread_mutex_destroy(&file_mutex);
    
    return 0;
}
