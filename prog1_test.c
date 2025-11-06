#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <semaphore.h>

// Структуры для товаров
typedef struct {
    int product_id;
    char name[50];
    float price;
} Product;

// Структуры для покупателей
typedef struct {
    int customer_id;
    char name[50];
    char email[50];
} Customer;

// Переменные
Product* products = NULL;
Customer* customers = NULL;
int products_count = 0;
int customers_count = 0;
sem_t* file_sem;
int sale_counter = 1;

//Загрузка товаров из файла products.txt
int load_products() {
    FILE* file = fopen("products.txt", "r");
    if (file == NULL) return 0;
    
    int count = 0;
    int capacity = 10;
    products = malloc(capacity * sizeof(Product)); // Выделяем память под товары
    
    char line[100];
    while (fgets(line, sizeof(line), file)) {
        if (count >= capacity) {
            capacity *= 2;
            products = realloc(products, capacity * sizeof(Product));
        }
        
        char name[50];
        if (sscanf(line, "%d %49s %f", &products[count].product_id, name, &products[count].price) == 3) {
            strcpy(products[count].name, name);
            count++;
        }
    }
    
    fclose(file);
    products_count = count;
    return count;
}

//Загрузка покупателей из файла customers.txt
int load_customers() {
    FILE* file = fopen("customers.txt", "r");
    if (file == NULL) return 0;
    
    int count = 0;
    int capacity = 10;
    customers = malloc(capacity * sizeof(Customer));
    
    char line[100];
    while (fgets(line, sizeof(line), file)) {
        if (count >= capacity) {
            capacity *= 2;
            customers = realloc(customers, capacity * sizeof(Customer));
        }
        
        char name[50], email[50];
        if (sscanf(line, "%d %49s %49s", &customers[count].customer_id, name, email) == 3) {
            strcpy(customers[count].name, name);
            strcpy(customers[count].email, email);
            count++;
        }
    }
    
    fclose(file);
    customers_count = count;
    return count;
}

//Получение текущего времени
void get_current_time(char* buffer) {
    time_t rawtime;
    struct tm* timeinfo;
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", timeinfo);
}

// Имитация продаж
void* simulate_sale(void* arg) {
    while (1) {
        sleep(rand() % 5 + 2); // Задержка
        
        int product_index = rand() % products_count;
        int customer_index = rand() % customers_count;
        
        // Получаем данные о продаже
        int sale_id = sale_counter++;
        char* customer_name = customers[customer_index].name;
        char* product_name = products[product_index].name;
        float price = products[product_index].price;
        char datetime[20];
        get_current_time(datetime);
        
        // Используем семафор для синхронизации доступа к файлу
        sem_wait(file_sem);
        
        // Записываем в файл о продажах
        FILE* file = fopen("sales.txt", "a");
        if (file != NULL) {
            fprintf(file, "%d|%s|%s|%s|%.2f\n",
                    sale_id, datetime, customer_name, product_name, price);
            fclose(file);
            printf("Продажа #%d: %s купил %s за %.2f\n",
                   sale_id, customer_name, product_name, price);
        }
        
        sem_post(file_sem);
    }
    return NULL;
}

int main() {
    srand(time(NULL));
    
    // Создаем именованный семафор
    file_sem = sem_open("/sales_sem", O_CREAT, 0644, 1);
    if (file_sem == SEM_FAILED) {
        perror("sem_open");
        return 1;
    }
    
    if (!load_products() || !load_customers()) {
        printf("Ошибка загрузки данных!\n");
        return 1;
    }
    
    printf("Имитация продаж запущена (%d товаров, %d покупателей)\n", products_count, customers_count);
    printf("Продажи записываются в файл: sales.txt (с названиями)\n");
    
    // Создаем 2 потока для имитации продаж
    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, simulate_sale, NULL);
    pthread_create(&thread2, NULL, simulate_sale, NULL);
    
    // Бесконечное ожидание
    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    
    // Закрываем семафор
    sem_close(file_sem);
    
    return 0;
}
