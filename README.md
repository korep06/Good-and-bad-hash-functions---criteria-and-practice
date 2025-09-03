# Good-and-bad-hash-functions---criteria-and-practice
**Цель урока:** Понять критерии оценки хеш-функций (равномерность, скорость), научиться анализировать их эффективность на практике и осознать компромисс между равномерностью распределения и скоростью вычисления.

---

#### 1. Ключевые термины и определения

- **Хорошая хеш-функция:** Функция, которая обладает двумя ключевыми свойствами:
    
    1. **Равномерность:** Равномерно распределяет ключи по всему диапазону возможных хеш-значений, минимизируя коллизии.
        
    2. **Скорость:** Вычисляется быстро, так как вызывается при каждой операции с контейнером.
        
- **Коллизия:** Ситуация, когда два разных ключа дают одинаковое значение хеш-функции.
    
- **Нагрузка (Load factor):** Отношение количества элементов в хеш-таблице к количеству корзин. Высокая нагрузка увеличивает вероятность коллизий.
    
- **`std::hash`:** Стандартная шаблонная структура в C++, предоставляющая хеш-функции для встроенных типов и некоторых стандартных классов (например, `std::string`).
    

---

#### 2. Основные концепции и теории

- **Влияние коллизий на производительность:** Высокий уровень коллизий деградирует производительность `unordered_set`/`unordered_map` до O(N) в худшем случае, так как поиск внутри корзины с большим количеством элементов становится линейным.
    
- **Компромисс между равномерностью и скоростью:** Идеальная хеш-функция должна быть и быстрой, и равномерной. На практике часто приходится искать баланс между этими двумя требованиями.
    
- **Анализ хеш-функции:** Чтобы оценить качество хеш-функции, нужно:
    
    1. Посчитать количество возможных уникальных ключей.
        
    2. Оценить количество возможных уникальных хешей, которые может сгенерировать функция.
        
    3. Сравнить эти два числа. Если количество хешей значительно меньше количества ключей — коллизий не избежать.
        
- **Пример с номерами:** Для `VehiclePlate` существует ~211 млн уникальных комбинаций. Хеш-функция, основанная только на цифрах (1000 значений), будет иметь катастрофически много коллизий. Функция, учитывающая цифры и регион (122 * 1000 = 122000 значений), уже лучше, но для 1 млн элементов нагрузка все еще высока (~8 элементов на корзину). Идеальная функция должна использовать _все_ данные номера.
    

---

#### 3. Синтаксис и код

**Пример плохой хеш-функции (только цифры):**

```cpp

struct PlateHasherTrivial {
    size_t operator()(const VehiclePlate& plate) const {
        return static_cast<size_t>(plate.GetDigits()); // ~1000 уникальных хешей
    }
};
```
// -> Много коллизий, низкая производительность.

**Пример улучшенной хеш-функции (цифры + регион):**

```cpp

struct PlateHasherRegion {
    size_t operator()(const VehiclePlate& plate) const {
        // ~122000 уникальных хешей
        return static_cast<size_t>(plate.GetDigits() + plate.GetRegion() * 1000);
    }
};
```
// -> Меньше коллизий, лучше производительность.

**Пример медленной, но равномерной хеш-функции (через строку):**


```cpp
struct PlateHasherString {
    size_t operator()(const VehiclePlate& plate) const {
        // Использует стандартный хешер для строки.
        // Равномерность высокая, но вычисление медленное из-за создания строки.
        return hasher_(plate.ToString());
    }
private:
    hash<string> hasher_;
};
```
// -> Мало коллизий, но низкая производительность из-за накладных расходов.

**Идеальный подход (быстрая и равномерная функция):**  
Нужно самостоятельно разработать функцию, которая быстро комбинирует все поля номера, не создавая промежуточных объектов вроде строки. Например, использовать побитовые операции:


```cpp
struct PlateHasherGood {
    size_t operator()(const VehiclePlate& plate) const {
        // Быстро комбинируем все данные номера
        size_t result = plate.GetRegion();
        result *= 1000;
        result += plate.GetDigits();
        result *= 100; // Умножаем на основание системы счисления для букв
        // Преобразуем буквы в числа (0-25) и добавляем к хешу
        result += (plate.GetLetters()[0] - 'A');
        result *= 100;
        result += (plate.GetLetters()[1] - 'A');
        result *= 100;
        result += (plate.GetLetters()[2] - 'A');
        return result;
    }
};
```
// -> Хороший баланс скорости и равномерности.

---

#### 4. Важные заметки и подводные камни

- **Измеряйте производительность.** Всегда тестируйте хеш-функции на реальных данных и с помощью профилирования (как в примере с `LOG_DURATION`). Теоретические рассуждения могут быть обманчивы.
    
- **Учитывайте предметную область.** Если данные имеют специфическое распределение (например, большинство номеров из одного региона), это может свести на нет преимущества простой хеш-функции. Функция должна быть устойчива к таким skew-ам.
    
- **Не забывайте о скорости.** Самая равномерная хеш-функция бесполезна, если она вычисляется медленно. Создание строк – дорогая операция.
    
- **Используйте стандартные хешеры для комбинирования.** Для создания сложных хеш-функций часто используют технику комбинирования хешей отдельных полей с помощью XOR и сдвигов. В C++ для этого можно использовать `boost::hash_combine` или реализовать аналогичную логику.
    
- **Помните о требованиях:** Для одного и того же объекта хеш должен всегда быть одинаковым. Если `a == b`, то и их хеши должны быть равны.

### Пример

```cpp
// Подключаем заголовочный файл для измерения времени выполнения
#include "log_duration.h"

#include <algorithm>
#include <array>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <random>
#include <set>
#include <string>
#include <sstream>
#include <unordered_set>

using namespace std;

// Класс для представления автомобильного номера
class VehiclePlate {
private:
    // Вспомогательный метод для сравнения номеров
    auto AsTuple() const {
        return tie(letters_, digits_, region_);
    }

public:
    // Операторы сравнения для использования в контейнерах
    bool operator==(const VehiclePlate& other) const {
        return AsTuple() == other.AsTuple();
    }

    bool operator<(const VehiclePlate& other) const {
        return AsTuple() < other.AsTuple();
    }

    // Конструктор автомобильного номера
    VehiclePlate(char l0, char l1, int digits, char l2, int region)
        : letters_{l0, l1, l2}
        , digits_(digits)
        , region_(region) {
    }

    // Преобразование номера в строку
    string ToString() const {
        ostringstream out;
        out << letters_[0] << letters_[1];
        // Форматируем цифры: 3 знака с ведущими нулями
        out << setfill('0') << right << setw(3) << digits_;
        out << letters_[2] << setw(2) << region_;

        return out.str();
    }

    // Геттеры для доступа к полям класса
    const array<char, 3>& GetLetters() const {
        return letters_;
    }

    int GetDigits() const {
        return digits_;
    }

    int GetRegion() const {
        return region_;
    }

private:
    array<char, 3> letters_;  // Буквы номера
    int digits_;               // Цифровая часть номера
    int region_;               // Код региона
};

// Тривиальная хеш-функция - использует только цифры
struct PlateHasherTrivial {
    size_t operator()(const VehiclePlate& plate) const {
        return static_cast<size_t>(plate.GetDigits());
    }
};

// Хеш-функция с учетом региона
struct PlateHasherRegion {
    size_t operator()(const VehiclePlate& plate) const {
        return static_cast<size_t>(plate.GetDigits() + plate.GetRegion() * 1000);
    }
};

// Хеш-функция на основе строкового представления
struct PlateHasherString {
    size_t operator()(const VehiclePlate& plate) const {
        return hasher(plate.ToString());
    }

    hash<string> hasher;
};

// Полноценная хеш-функция, учитывающая все поля номера
struct PlateHasherAll {
    size_t operator()(const VehiclePlate& plate) const {
        size_t hasher_all = 0;
        
        // Хешируем буквы с помощью полиномиального хеширования
        for (auto c : plate.GetLetters()) {
            hasher_all = hasher_all * 31 + static_cast<size_t>(c);
        }
        
        // Добавляем цифровую часть
        hasher_all = hasher_all * 31 + static_cast<size_t>(plate.GetDigits());
        
        // Добавляем регион
        hasher_all = hasher_all * 31 + static_cast<size_t>(plate.GetRegion());

        return hasher_all;
    }
};

// Перегрузка оператора вывода для VehiclePlate
ostream& operator<<(ostream& out, VehiclePlate plate) {
    out << plate.ToString();
    return out;
}

// Генератор случайных автомобильных номеров
class PlateGenerator {
    char GenerateChar() {
        uniform_int_distribution<short> char_gen{0, static_cast<short>(possible_chars_.size() - 1)};
        return possible_chars_[char_gen(engine_)];
    }

    int GenerateNumber() {
        uniform_int_distribution<short> num_gen{0, 999};
        return num_gen(engine_);
    }

    int GenerateRegion() {
        uniform_int_distribution<short> region_gen{0, static_cast<short>(possible_regions_.size() - 1)};
        return possible_regions_[region_gen(engine_)];
    }

public:
    VehiclePlate Generate() {
        return VehiclePlate(GenerateChar(), GenerateChar(), GenerateNumber(), GenerateChar(), GenerateRegion());
    }

private:
    mt19937 engine_;  // Генератор случайных чисел

    // Допустимые коды регионов (inline static для инициализации внутри класса)
    inline static const array possible_regions_ = {
        1, 2, 102, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 113, 14, 15, 16, 116, 17, 18,
        19, 20, 21, 121, 22, 23, 93, 123, 24, 84, 88, 124, 25, 125, 26, 27, 28, 29, 30,
        31, 32, 33, 34, 35, 36, 136, 37, 38, 85, 39, 91, 40, 41, 82, 42, 142, 43, 44, 45,
        46, 47, 48, 49, 50, 90, 150, 190, 51, 52, 152, 53, 54, 154, 55, 56, 57, 58, 59,
        81, 159, 60, 61, 161, 62, 63, 163, 64, 164, 65, 66, 96, 67, 68, 69, 70, 71, 72,
        73, 173, 74, 174, 75, 80, 76, 77, 97, 99, 177, 199, 197, 777, 78, 98, 178, 79,
        83, 86, 87, 89, 94, 95
    };

    // Допустимые буквы для номеров
    inline static const string_view possible_chars_ = "ABCEHKMNOPTXY"sv;
};

int main() {
    static const int N = 1'000'000;  // Количество генерируемых номеров

    PlateGenerator generator;
    vector<VehiclePlate> fill_vector;  // Вектор для заполнения контейнера
    vector<VehiclePlate> find_vector;  // Вектор для поиска в контейнере

    // Генерируем N номеров для заполнения контейнера
    generate_n(back_inserter(fill_vector), N, [&]() {
        return generator.Generate();
    });
    
    // Генерируем N номеров для поиска
    generate_n(back_inserter(find_vector), N, [&]() {
        return generator.Generate();
    });

    int found;
    
    // Тестирование скорости unordered_set с полноценной хеш-функцией
    {
        LOG_DURATION("unordered_set");
        unordered_set<VehiclePlate, PlateHasherAll> container;
        
        // Заполняем контейнер номерами
        for (auto& p : fill_vector) {
            container.insert(p);
        }
        
        // Подсчитываем количество найденных совпадений
        found = count_if(find_vector.begin(), find_vector.end(), [&](const VehiclePlate& plate) {
            return container.count(plate) > 0;
        });
    }
    cout << "Found matches (1): "s << found << endl;

    // Тестирование скорости set (красно-черное дерево)
    {
        LOG_DURATION("set");
        set<VehiclePlate> container;
        
        // Заполняем контейнер номерами
        for (auto& p : fill_vector) {
            container.insert(p);
        }
        
        // Подсчитываем количество найденных совпадений
        found = count_if(find_vector.begin(), find_vector.end(), [&](const VehiclePlate& plate) {
            return container.count(plate) > 0;
        });
    }
    cout << "Found matches (2): "s << found << endl;
}
```


```сpp
#pragma once


#include <chrono>
#include <iostream>


#define PROFILE_CONCAT_INTERNAL(X, Y) X##Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)

  
class LogDuration {
public:
    // заменим имя типа std::chrono::steady_clock
    // с помощью using для удобства
    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string& id) : id_(id) {
    }

  
    ~LogDuration() {
        using namespace std::chrono;
        using namespace std::literals;
  
        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;

        std::cerr << id_ << ": "s << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:
    const std::string id_;
    const Clock::time_point start_time_ = Clock::now();
};
```
