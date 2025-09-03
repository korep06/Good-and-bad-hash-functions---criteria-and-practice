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
