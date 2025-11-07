#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

// Структура для хранения данных о продаже
typedef struct {
    int sale_id;
    char datetime[20];
    char customer_name[50];
    char product_name[50];
    float price;
} Sale;

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
Sale* sales = NULL;
Product* products = NULL;
Customer* customers = NULL;
int sales_count = 0, products_count = 0, customers_count = 0;
pthread_mutex_t file_mutex = PTHREAD_MUTEX_INITIALIZER;

// Переменные для управления обработкой
int processing_enabled = 1;
float min_customer_sum = 10000.0;

int load_products() {
    FILE* file = fopen("products.txt", "r");
    if (file == NULL) return 0;
    
    int count = 0;
    int capacity = 10;
    products = malloc(capacity * sizeof(Product));
    
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

void update_sales_data() {
    pthread_mutex_lock(&file_mutex);
    
    FILE* file = fopen("sales.txt", "r");
    if (file != NULL) {
        // Считаем количество строк
        int count = 0;
        char buffer[256];
        while (fgets(buffer, sizeof(buffer), file)) {
            count++;
        }
        
        // Перематываем и читаем данные
        rewind(file);
        
        if (sales != NULL) free(sales);
        sales = malloc(count * sizeof(Sale));
        sales_count = 0;
        
        for (int i = 0; i < count; i++) {
            if (fgets(buffer, sizeof(buffer), file)) {
                char datetime[20], customer_name[50], product_name[50];
                if (sscanf(buffer, "%d|%19[^|]|%49[^|]|%49[^|]|%f",
                          &sales[sales_count].sale_id,
                          datetime,
                          customer_name,
                          product_name,
                          &sales[sales_count].price) == 5) {
                    strcpy(sales[sales_count].datetime, datetime);
                    strcpy(sales[sales_count].customer_name, customer_name);
                    strcpy(sales[sales_count].product_name, product_name);
                    sales_count++;
                }
            }
        }
        fclose(file);
    } else {
        sales_count = 0;
        if (sales != NULL) free(sales);
        sales = NULL;
    }
    
    pthread_mutex_unlock(&file_mutex);
}

// 1. Общая сумма продаж
void* calculate_total_sales(void* arg) {
    while (1) {
        if (processing_enabled) {
            update_sales_data();
            
            float total = 0;
            for (int i = 0; i < sales_count; i++) {
                total += sales[i].price;
            }
            
            FILE* report = fopen("total_sales_report.txt", "w");
            if (report != NULL) {
                fprintf(report, "ОТЧЕТ: ОБЩАЯ СУММА ПРОДАЖ\n");
                fprintf(report, "---------------------------\n");
                fprintf(report, "Общая сумма: %.2f\n", total);
                fprintf(report, "Кол-во продаж: %d\n", sales_count);
                fprintf(report, "Средняя сумма: %.2f\n", sales_count > 0 ? total / sales_count : 0);
                fclose(report);
                
                printf("[Общая сумма] Продаж: %d, Сумма: %.2f\n", sales_count, total);
            }
        }
        sleep(5);
    }
    return NULL;
}

// 2. Популярные и непопулярные товары
void* analyze_product_popularity(void* arg) {
    while (1) {
        if (processing_enabled) {
            update_sales_data();
            
            // Считаем продажи по названиям товаров
            int* product_sales = calloc(products_count, sizeof(int));
            float* product_revenue = calloc(products_count, sizeof(float));
            
            // Подсчитываем продажи для каждого товара
            for (int i = 0; i < sales_count; i++) {
                for (int j = 0; j < products_count; j++) {
                    if (strcmp(sales[i].product_name, products[j].name) == 0) {
                        product_sales[j]++;
                        product_revenue[j] += sales[i].price;
                        break;
                    }
                }
            }
            
            // Сортировка по популярности
            int* indices = malloc(products_count * sizeof(int));
            for (int i = 0; i < products_count; i++) indices[i] = i;
            
            for (int i = 0; i < products_count - 1; i++) {
                for (int j = i + 1; j < products_count; j++) {
                    if (product_sales[indices[i]] < product_sales[indices[j]]) {
                        int temp = indices[i];
                        indices[i] = indices[j];
                        indices[j] = temp;
                    }
                }
            }
            
            FILE* report = fopen("product_popularity_report.txt", "w");
            if (report != NULL) {
                fprintf(report, "ОТЧЕТ: ПОПУЛЯРНОСТЬ ТОВАРОВ\n");
                fprintf(report, "----------------------------\n");
                
                fprintf(report, "\nТОП-5 популярных товаров:\n");
                for (int i = 0; i < 5 && i < products_count; i++) {
                    int idx = indices[i];
                    fprintf(report, "%d. %s - %d продаж, выручка: %.2f\n",
                            i+1, products[idx].name, product_sales[idx], product_revenue[idx]);
                }
                
                fprintf(report, "\nТОП-5 непопулярных товаров:\n");
                for (int i = products_count - 1; i >= products_count - 5 && i >= 0; i--) {
                    int idx = indices[i];
                    fprintf(report, "%d. %s - %d продаж, выручка: %.2f\n",
                            products_count - i, products[idx].name, product_sales[idx], product_revenue[idx]);
                }
                
                fclose(report);
                printf("[Популярность] Анализ завершен\n");
            }
            
            free(product_sales);
            free(product_revenue);
            free(indices);
        }
        sleep(7);
    }
    return NULL;
}

// 3. Покупатели с большой суммой покупок
void* find_top_customers(void* arg) {
    while (1) {
        if (processing_enabled) {
            update_sales_data();
            
            // Считаем суммы по именам покупателей
            float* customer_totals = calloc(customers_count, sizeof(float));
            
            for (int i = 0; i < sales_count; i++) {
                for (int j = 0; j < customers_count; j++) {
                    if (strcmp(sales[i].customer_name, customers[j].name) == 0) {
                        customer_totals[j] += sales[i].price;
                        break;
                    }
                }
            }
            
            FILE* report = fopen("top_customers_report.txt", "w");
            if (report != NULL) {
                fprintf(report, "ОТЧЕТ: ПОКУПАТЕЛИ С СУММОЙ > %.2f\n", min_customer_sum);
                fprintf(report, "------------------------------------\n");
                
                int count = 0;
                for (int i = 0; i < customers_count; i++) {
                    if (customer_totals[i] > min_customer_sum) {
                        fprintf(report, "%d. %s - %.2f\n",
                                ++count, customers[i].name, customer_totals[i]);
                    }
                }
                
                if (count == 0) {
                    fprintf(report, "Нет покупателей с суммой выше %.2f\n", min_customer_sum);
                }
                
                fclose(report);
                printf("[Покупатели] Найдено %d покупателей с суммой > %.2f\n", count, min_customer_sum);
            }
            
            free(customer_totals);
        }
        sleep(9);
    }
    return NULL;
}

// 4. Анализ тенденций продаж
void* analyze_sales_trends(void* arg) {
    while (1) {
        if (processing_enabled) {
            update_sales_data();
            
            // Анализ по часам
            float hourly_sales[24] = {0};
            int hourly_count[24] = {0};
            
            for (int i = 0; i < sales_count; i++) {
                int hour;
                sscanf(sales[i].datetime, "%*d-%*d-%*d %d", &hour);
                if (hour >= 0 && hour < 24) {
                    hourly_sales[hour] += sales[i].price;
                    hourly_count[hour]++;
                }
            }
            
            FILE* report = fopen("sales_trends_report.txt", "w");
            if (report != NULL) {
                fprintf(report, "ОТЧЕТ: ТЕНДЕНЦИИ ПРОДАЖ\n");
                fprintf(report, "-------------------------\n");
                
                // Находим час с максимальными продажами
                int max_hour = 0;
                float max_sales = 0;
                for (int i = 0; i < 24; i++) {
                    if (hourly_sales[i] > max_sales) {
                        max_sales = hourly_sales[i];
                        max_hour = i;
                    }
                }
                
                fprintf(report, "Пиковый час продаж: %02d:00 (%.2f)\n", max_hour, max_sales);
                fprintf(report, "\nПродажи по часам:\n");
                for (int i = 0; i < 24; i++) {
                    if (hourly_count[i] > 0) {
                        fprintf(report, "%02d:00 - %d продаж, сумма: %.2f\n",
                                i, hourly_count[i], hourly_sales[i]);
                    }
                }
                
                fclose(report);
                printf("[Тренды] Пиковый час: %02d:00\n", max_hour);
            }
        }
        sleep(11);
    }
    return NULL;
}

// 5. Пользовательский интерфейс для управления параметрами
void* user_interface_thread(void* arg) {
    printf("\n=== УПРАВЛЕНИЕ ОБРАБОТКОЙ ===\n");
    printf("Команды:\n");
    printf("  s - остановить обработку\n");
    printf("  r - возобновить обработку\n");
    printf("  m - изменить минимальную сумму\n");
    printf("  q - выйти\n");
    
    char command;
    while (1) {
        printf("\n> ");
        scanf(" %c", &command);
        
        switch (command) {
            case 's':
                processing_enabled = 0;
                printf("Обработка остановлена\n");
                break;
                
            case 'r':
                processing_enabled = 1;
                printf("Обработка возобновлена\n");
                break;
                
            case 'm':
                printf("Текущая минимальная сумма: %.2f\n", min_customer_sum);
                printf("Новая сумма: ");
                scanf("%f", &min_customer_sum);
                printf("Установлена сумма: %.2f\n", min_customer_sum);
                break;
                
            case 'q':
                printf("Выход...\n");
                // Освобождаем память перед выходом
                if (sales != NULL) free(sales);
                if (products != NULL) free(products);
                if (customers != NULL) free(customers);
                exit(0);
                break;
                
            default:
                printf("Неизвестная команда\n");
        }
    }
    return NULL;
}

int main() {
    if (!load_products() || !load_customers()) {
        printf("Ошибка загрузки данных!\n");
        return 1;
    }
    
    printf("Обработка данных запущена\n");
    printf("Загружено: %d товаров, %d покупателей\n", products_count, customers_count);
    printf("Отчеты сохраняются в файлы:\n");
    printf("  - total_sales_report.txt\n");
    printf("  - product_popularity_report.txt\n");
    printf("  - top_customers_report.txt\n");
    printf("  - sales_trends_report.txt\n");
    
    // Запускаем ВСЕ 5 потоков
    pthread_t threads[5];
    pthread_create(&threads[0], NULL, calculate_total_sales, NULL);
    pthread_create(&threads[1], NULL, analyze_product_popularity, NULL);
    pthread_create(&threads[2], NULL, find_top_customers, NULL);
    pthread_create(&threads[3], NULL, analyze_sales_trends, NULL);
    pthread_create(&threads[4], NULL, user_interface_thread, NULL);
    
    // Ожидаем все потоки
    for (int i = 0; i < 5; i++) {
        pthread_join(threads[i], NULL);
    }
    
    return 0;
}
