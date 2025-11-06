#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LENGTH 100
#define TOP_N 5

// Структура для покупателя
typedef struct {
    int code;
    char name[MAX_LENGTH];
    char phone[MAX_LENGTH];
} Customer;

// Структура для товара
typedef struct {
    int code;
    char name[MAX_LENGTH];
    float price;
    int quantity;
    int sales_count;  // Добавлено для подсчета популярности
    float total_revenue;  // Добавлено для общей выручки по товару
} Product;

// Структура для продажи
typedef struct {
    int sale_id;
    char datetime[MAX_LENGTH];
    int customer_code;
    int product_code;
} Sale;

// Структура для статистики покупателей
typedef struct {
    int customer_code;
    char customer_name[MAX_LENGTH];
    float total_spent;
} CustomerStats;

// Функции для работы с файлами (остаются без изменений)
int read_customers(const char* filename, Customer** customers) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Ошибка открытия файла покупателей\n");
        return 0;
    }
    
    int count = 0;
    char buffer[256];
    
    // Пропускаем заголовок если есть
    fgets(buffer, sizeof(buffer), file);
    
    while (fgets(buffer, sizeof(buffer), file)) {
        count++;
    }
    
    rewind(file);
    *customers = malloc(count * sizeof(Customer));
    
    // Пропускаем заголовок если есть
    fgets(buffer, sizeof(buffer), file);
    
    for (int i = 0; i < count; i++) {
        if (fgets(buffer, sizeof(buffer), file)) {
            sscanf(buffer, "%d %s %s", 
                   &(*customers)[i].code, 
                   (*customers)[i].name, 
                   (*customers)[i].phone);
        }
    }
    
    fclose(file);
    return count;
}

int read_products(const char* filename, Product** products) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Ошибка открытия файла товаров\n");
        return 0;
    }
    
    int count = 0;
    char buffer[256];
    
    while (fgets(buffer, sizeof(buffer), file)) {
        count++;
    }
    
    rewind(file);
    *products = malloc(count * sizeof(Product));
    
    for (int i = 0; i < count; i++) {
        if (fgets(buffer, sizeof(buffer), file)) {
            sscanf(buffer, "%d %s %f %d", 
                   &(*products)[i].code, 
                   (*products)[i].name, 
                   &(*products)[i].price, 
                   &(*products)[i].quantity);
            (*products)[i].sales_count = 0;
            (*products)[i].total_revenue = 0;
        }
    }
    
    fclose(file);
    return count;
}

int read_sales(const char* filename, Sale** sales) {
    FILE* file = fopen(filename, "r");
    if (!file) {
        printf("Ошибка открытия файла продаж\n");
        return 0;
    }
    
    int count = 0;
    char buffer[256];
    
    // Пропускаем заголовок если есть
    fgets(buffer, sizeof(buffer), file);
    
    while (fgets(buffer, sizeof(buffer), file)) {
        count++;
    }
    
    rewind(file);
    *sales = malloc(count * sizeof(Sale));
    
    // Пропускаем заголовок если есть
    fgets(buffer, sizeof(buffer), file);
    
    for (int i = 0; i < count; i++) {
        if (fgets(buffer, sizeof(buffer), file)) {
            sscanf(buffer, "%d %s %d %d", 
                   &(*sales)[i].sale_id, 
                   (*sales)[i].datetime, 
                   &(*sales)[i].customer_code, 
                   &(*sales)[i].product_code);
        }
    }
    
    fclose(file);
    return count;
}

// Поиск покупателя по коду
Customer* find_customer_by_code(Customer* customers, int count, int code) {
    for (int i = 0; i < count; i++) {
        if (customers[i].code == code) {
            return &customers[i];
        }
    }
    return NULL;
}

// Поиск товара по коду
Product* find_product_by_code(Product* products, int count, int code) {
    for (int i = 0; i < count; i++) {
        if (products[i].code == code) {
            return &products[i];
        }
    }
    return NULL;
}

// Функция для сравнения товаров по количеству продаж (для qsort)
int compare_products_by_sales(const void* a, const void* b) {
    const Product* pa = (const Product*)a;
    const Product* pb = (const Product*)b;
    return pb->sales_count - pa->sales_count;  // По убыванию
}

// Функция для сравнения покупателей по потраченной сумме (для qsort)
int compare_customers_by_spent(const void* a, const void* b) {
    const CustomerStats* ca = (const CustomerStats*)a;
    const CustomerStats* cb = (const CustomerStats*)b;
    if (cb->total_spent > ca->total_spent) return 1;
    if (cb->total_spent < ca->total_spent) return -1;
    return 0;
}

// 1. Определение самых популярных и непопулярных товаров
void analyze_product_popularity(Product* products, int products_count, 
                               Sale* sales, int sales_count) {
    printf("\n=== АНАЛИЗ ПОПУЛЯРНОСТИ ТОВАРОВ ===\n");
    
    // Сбрасываем счетчики
    for (int i = 0; i < products_count; i++) {
        products[i].sales_count = 0;
        products[i].total_revenue = 0;
    }
    
    // Подсчитываем продажи для каждого товара
    for (int i = 0; i < sales_count; i++) {
        Product* product = find_product_by_code(products, products_count, 
                                               sales[i].product_code);
        if (product) {
            product->sales_count++;
            product->total_revenue += product->price;
        }
    }
    
    // Сортируем товары по количеству продаж
    Product* sorted_products = malloc(products_count * sizeof(Product));
    memcpy(sorted_products, products, products_count * sizeof(Product));
    qsort(sorted_products, products_count, sizeof(Product), compare_products_by_sales);
    
    // Выводим топ-5 самых популярных товаров
    printf("\n--- Топ-%d самых популярных товаров ---\n", TOP_N);
    printf("Место\tТовар\t\tКол-во продаж\tВыручка\n");
    for (int i = 0; i < TOP_N && i < products_count; i++) {
        printf("%d\t%-12s\t%d\t\t%.2f\n", 
               i + 1, 
               sorted_products[i].name, 
               sorted_products[i].sales_count,
               sorted_products[i].total_revenue);
    }
    
    // Выводим топ-5 самых непопулярных товаров
    printf("\n--- Топ-%d самых непопулярных товаров ---\n", TOP_N);
    printf("Место\tТовар\t\tКол-во продаж\tВыручка\n");
    for (int i = 0; i < TOP_N && i < products_count; i++) {
        int reverse_index = products_count - 1 - i;
        if (reverse_index >= 0) {
            printf("%d\t%-12s\t%d\t\t%.2f\n", 
                   i + 1, 
                   sorted_products[reverse_index].name, 
                   sorted_products[reverse_index].sales_count,
                   sorted_products[reverse_index].total_revenue);
        }
    }
    
    free(sorted_products);
}

// 2. Определение покупателей, набравших товаров на сумму более заданной
void find_top_customers_by_spent(Customer* customers, int customers_count,
                                Product* products, int products_count,
                                Sale* sales, int sales_count) {
    printf("\n=== ПОИСК КРУПНЫХ ПОКУПАТЕЛЕЙ ===\n");
    
    float min_amount;
    printf("Введите минимальную сумму покупок: ");
    scanf("%f", &min_amount);
    
    // Создаем массив для статистики по покупателям
    CustomerStats* customer_stats = malloc(customers_count * sizeof(CustomerStats));
    
    // Инициализируем статистику
    for (int i = 0; i < customers_count; i++) {
        customer_stats[i].customer_code = customers[i].code;
        strcpy(customer_stats[i].customer_name, customers[i].name);
        customer_stats[i].total_spent = 0;
    }
    
    // Подсчитываем суммы покупок для каждого покупателя
    for (int i = 0; i < sales_count; i++) {
        Customer* customer = find_customer_by_code(customers, customers_count, 
                                                  sales[i].customer_code);
        Product* product = find_product_by_code(products, products_count, 
                                               sales[i].product_code);
        
        if (customer && product) {
            // Находим индекс покупателя в статистике
            for (int j = 0; j < customers_count; j++) {
                if (customer_stats[j].customer_code == customer->code) {
                    customer_stats[j].total_spent += product->price;
                    break;
                }
            }
        }
    }
    
    // Сортируем покупателей по потраченной сумме
    qsort(customer_stats, customers_count, sizeof(CustomerStats), 
          compare_customers_by_spent);
    
    // Выводим покупателей с суммой покупок больше заданной
    printf("\nПокупатели с суммой покупок более %.2f:\n", min_amount);
    printf("Покупатель\tОбщая сумма\n");
    int found = 0;
    for (int i = 0; i < customers_count; i++) {
        if (customer_stats[i].total_spent > min_amount) {
            printf("%s\t\t%.2f\n", 
                   customer_stats[i].customer_name, 
                   customer_stats[i].total_spent);
            found = 1;
        }
    }
    
    if (!found) {
        printf("Покупателей с суммой покупок более %.2f не найдено.\n", min_amount);
    }
    
    // Также выводим общий топ покупателей
    printf("\n--- Общий рейтинг покупателей ---\n");
    printf("Место\tПокупатель\tОбщая сумма\n");
    for (int i = 0; i < customers_count && i < TOP_N; i++) {
        printf("%d\t%s\t\t%.2f\n", 
               i + 1, 
               customer_stats[i].customer_name, 
               customer_stats[i].total_spent);
    }
    
    free(customer_stats);
}

// Основная функция вывода деталей продаж (остается без изменений)
void print_sales_details(Sale* sales, int sales_count, 
                        Customer* customers, int customers_count,
                        Product* products, int products_count) {
    printf("=== ДЕТАЛЬНАЯ ИНФОРМАЦИЯ О ПРОДАЖАХ ===\n");
    printf("ID\tДата/Время\t\tПокупатель\tТовар\tЦена\n");
    printf("------------------------------------------------------------\n");
    
    float total_revenue = 0;
    
    for (int i = 0; i < sales_count; i++) {
        Customer* customer = find_customer_by_code(customers, customers_count, 
                                                  sales[i].customer_code);
        Product* product = find_product_by_code(products, products_count, 
                                               sales[i].product_code);
        
        if (customer && product) {
            printf("%d\t%s\t%s\t%s\t%.2f\n", 
                   sales[i].sale_id,
                   sales[i].datetime,
                   customer->name,
                   product->name,
                   product->price);
            
            total_revenue += product->price;
        }
    }
    
    printf("------------------------------------------------------------\n");
    printf("Общая выручка: %.2f\n", total_revenue);
}

int main() {
    Customer* customers = NULL;
    Product* products = NULL;
    Sale* sales = NULL;
    
    // Чтение данных из файлов
    int customers_count = read_customers("customers.txt", &customers);
    int products_count = read_products("products.txt", &products);
    int sales_count = read_sales("sales.txt", &sales);
    
    if (customers_count == 0 || products_count == 0 || sales_count == 0) {
        printf("Ошибка загрузки данных\n");
        return 1;
    }
    
    printf("Загружено:\n");
    printf("- Покупателей: %d\n", customers_count);
    printf("- Товаров: %d\n", products_count);
    printf("- Продаж: %d\n\n", sales_count);
    
    // Обработка и вывод данных
    print_sales_details(sales, sales_count, customers, customers_count, 
                       products, products_count);
    
    // Новые функции анализа
    analyze_product_popularity(products, products_count, sales, sales_count);
    find_top_customers_by_spent(customers, customers_count, products, products_count, 
                               sales, sales_count);
    
    // Освобождение памяти
    free(customers);
    free(products);
    free(sales);
    
    return 0;
}
