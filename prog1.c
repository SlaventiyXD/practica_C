#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <stdatomic.h>

#define MAX_PRODUCTS 10
#define MAX_PRODUCT_NAME 50
#define SALES_FILE "sales.txt"
#define CUSTOMERS_FILE "customers.txt"
#define PRODUCTS_FILE "products.txt"

// Атомарная переменная для контроля работы потоков
atomic_int keep_running = 1;

// Структуры данных
typedef struct {
    int code;
    char name[MAX_PRODUCT_NAME];
    float price;
    int quantity;
} Product;

typedef struct {
    int code;
    char name[MAX_PRODUCT_NAME];
    char phone[15];
} Customer;

// Структура для передачи данных в поток
typedef struct {
    int thread_id;
    int product_code;
    int customer_code;
    int min_delay;
    int max_delay;
} thread_data_t;

// Мьютекс для синхронизации доступа к файлу
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Глобальные массивы товаров и покупателей
Product products[] = {
    {1, "Ноутбук", 50000.00, 10},
    {2, "Смартфон", 35000.50, 15},
    {3, "Наушники", 4500.75, 20},
    {4, "Планшет", 28000.25, 8}
};

Customer customers[] = {
    {1, "Иванов", "+79991234567"},
    {2, "Петров", "+79992345678"},
    {3, "Сидоров", "+79993456789"},
    {4, "Козлов", "+79994567890"}
};

// Обработчик сигнала Ctrl+C
void signal_handler(int sig) {
    if (sig == SIGINT) {
        printf("\nПолучен сигнал Ctrl+C. Завершение работы...\n");
        atomic_store(&keep_running, 0);
    }
}

// Функция для получения текущей даты и времени в формате ГГГГ-ММ-ДД_ЧЧ:ММ:СС
void get_current_datetime(char* buffer, int buffer_size) {
    time_t rawtime;
    struct tm* timeinfo;
    
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    
    strftime(buffer, buffer_size, "%Y-%m-%d_%H:%M:%S", timeinfo);
}

// Функция для инициализации файлов
void initialize_files() {
    // Очищаем файл продаж
    FILE* file = fopen(SALES_FILE, "w");
    if (file != NULL) {
        fclose(file);
        printf("Файл продаж инициализирован: %s\n", SALES_FILE);
    }
    
    // Создаем файл покупателей
    file = fopen(CUSTOMERS_FILE, "w");
    if (file != NULL) {
        for (int i = 0; i < sizeof(customers) / sizeof(customers[0]); i++) {
            fprintf(file, "%d %s %s\n", 
                    customers[i].code, 
                    customers[i].name, 
                    customers[i].phone);
        }
        fclose(file);
        printf("Файл покупателей создан: %s\n", CUSTOMERS_FILE);
    }
    
    // Создаем файл товаров
    file = fopen(PRODUCTS_FILE, "w");
    if (file != NULL) {
        for (int i = 0; i < sizeof(products) / sizeof(products[0]); i++) {
            fprintf(file, "%d %s %.2f %d\n", 
                    products[i].code, 
                    products[i].name, 
                    products[i].price, 
                    products[i].quantity);
        }
        fclose(file);
        printf("Файл товаров создан: %s\n", PRODUCTS_FILE);
    }
}

// Функция потока - имитация продаж
void* sales_thread(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    // Находим информацию о товаре и покупателе
    Product* product = NULL;
    Customer* customer = NULL;
    
    for (int i = 0; i < sizeof(products) / sizeof(products[0]); i++) {
        if (products[i].code == data->product_code) {
            product = &products[i];
            break;
        }
    }
    
    for (int i = 0; i < sizeof(customers) / sizeof(customers[0]); i++) {
        if (customers[i].code == data->customer_code) {
            customer = &customers[i];
            break;
        }
    }
    
    if (!product || !customer) {
        printf("Ошибка: товар или покупатель не найден!\n");
        return NULL;
    }
    
    printf("Поток %d начал продажи: %s для %s\n", 
           data->thread_id, product->name, customer->name);
    
    int sale_counter = 0;
    
    while (atomic_load(&keep_running)) {
        // Генерируем случайную задержку
        int delay = data->min_delay + rand() % (data->max_delay - data->min_delay + 1);
        
        // Используем usleep для более быстрого отклика на Ctrl+C
        for (int i = 0; i < delay && atomic_load(&keep_running); i++) {
            sleep(1);
        }
        
        // Проверяем, не нужно ли завершать работу
        if (!atomic_load(&keep_running)) {
            break;
        }
        
        // Получаем текущее время
        char datetime[20];
        get_current_datetime(datetime, sizeof(datetime));
        
        // Блокируем мьютекс для записи в файл
        pthread_mutex_lock(&file_mutex);
        
        FILE* file = fopen(SALES_FILE, "a");
        if (file != NULL) {
            // Генерируем ID продажи (thread_id * 1000 + счетчик)
            int sale_id = data->thread_id * 1000 + (sale_counter + 1);
            
            // Записываем в формате: ID_продажи Дата_время Код_покупателя Код_товара
            fprintf(file, "%d %s %d %d\n", 
                    sale_id, 
                    datetime, 
                    data->customer_code, 
                    data->product_code);
            fclose(file);
            
            printf("Поток %d: Продажа %d | %s купил %s | Задержка: %d сек\n", 
                   data->thread_id, sale_counter + 1, customer->name, product->name, delay);
            
            sale_counter++;
        } else {
            printf("Ошибка открытия файла!\n");
        }
        
        // Разблокируем мьютекс
        pthread_mutex_unlock(&file_mutex);
    }
    
    printf("Поток %d завершил продажи: %s для %s (всего продаж: %d)\n", 
           data->thread_id, product->name, customer->name, sale_counter);
    
    return NULL;
}

int main() {
    // Инициализация генератора случайных чисел
    srand(time(NULL));
    
    // Устанавливаем обработчик сигнала Ctrl+C
    signal(SIGINT, signal_handler);
    
    // Инициализируем файлы
    initialize_files();
    
    // Данные для потоков (комбинации покупатель-товар)
    thread_data_t thread_data[] = {
        {1, 1, 1, 2, 5},  // Иванов покупает Ноутбук
        {2, 2, 2, 1, 3},  // Петров покупает Смартфон
        {3, 3, 3, 1, 4},  // Сидоров покупает Наушники
        {4, 4, 4, 3, 6},  // Козлов покупает Планшет
        {5, 1, 2, 2, 4},  // Петров покупает Ноутбук
        {6, 2, 1, 1, 3}   // Иванов покупает Смартфон
    };
    
    int num_threads = sizeof(thread_data) / sizeof(thread_data[0]);
    pthread_t threads[num_threads];
    
    printf("\nЗапуск %d потоков продаж...\n", num_threads);
    printf("Формат записи: ID_продажи Дата_время Код_покупателя Код_товара\n");
    printf("Для остановки программы нажмите Ctrl+C\n");
    printf("==================================================\n");
    
    // Создаем потоки
    for (int i = 0; i < num_threads; i++) {
        if (pthread_create(&threads[i], NULL, sales_thread, &thread_data[i]) != 0) {
            perror("Ошибка создания потока");
            return 1;
        }
    }
    
    // Ожидаем завершения всех потоков (по сигналу Ctrl+C)
    for (int i = 0; i < num_threads; i++) {
        if (pthread_join(threads[i], NULL) != 0) {
            perror("Ошибка ожидания потока");
            return 1;
        }
    }
    
    printf("==================================================\n");
    printf("Все потоки завершены!\n");
    printf("Созданные файлы:\n");
    printf("- %s (продажи)\n", SALES_FILE);
    printf("- %s (покупатели)\n", CUSTOMERS_FILE);
    printf("- %s (товары)\n", PRODUCTS_FILE);
    
    // Выводим содержимое файла продаж
    printf("\nСодержимое файла продаж:\n");
    printf("------------------------\n");
    FILE* file = fopen(SALES_FILE, "r");
    if (file != NULL) {
        char line[256];
        int line_count = 0;
        while (fgets(line, sizeof(line), file)) {
            printf("%s", line);
            line_count++;
        }
        fclose(file);
        printf("\nВсего записей о продажах: %d\n", line_count);
    } else {
        printf("Не удалось открыть файл продаж\n");
    }
    
    // Уничтожаем мьютекс
    pthread_mutex_destroy(&file_mutex);
    
    return 0;
}
