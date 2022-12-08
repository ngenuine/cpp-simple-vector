#pragma once

#include <cassert>
#include <stdexcept>
#include <initializer_list>
#include <iostream>
#include <utility>
#include <iterator>

#include "array_ptr.h"

class ReserveProxyObj {
public:
    ReserveProxyObj(size_t capacity) : capacity_(capacity)
    {
    }

    size_t GetCapacity() {
        return capacity_;
    }

private:
    size_t capacity_ = 0;
};

ReserveProxyObj Reserve(size_t capacity_to_reserve) {
    return ReserveProxyObj(capacity_to_reserve);
}

template <typename Type>
class SimpleVector {
// внесите в класс SimpleVector необходимые изменения для поддержки move-семантики
public:
    using Iterator = Type*;
    using ConstIterator = const Type*;

    SimpleVector() noexcept = default;

    SimpleVector(ReserveProxyObj reserver) {
        SimpleVector tmp(reserver.GetCapacity());
        this->swap(tmp);
        size_ = 0;
    }

    // конструктор копирования
    SimpleVector(const SimpleVector& other) {
        ArrayPtr<Type> temp(other.GetSize()); // было other.GetCapacity(), но зачем пустое место резервировать? -- я буду переносить только то, что в нем сейчас есть
        std::copy(other.begin(), other.end(), &temp[0]);
        simple_v_.swap(temp);

        size_ = other.GetSize();
        capacity_ = other.GetCapacity();
    }

 
    // оператор присваивания с защитой от самоприсваивания
    SimpleVector& operator=(const SimpleVector& rhs) {
        if (this != &rhs) {
            auto rhs_copy(rhs);
            swap(rhs_copy);
        }

        return *this;
    }

    // это нужно если мне целый вектор перемещаемых объектов дадут переместить через SimpleVector new_vec(moving_vec);
    SimpleVector(SimpleVector&& other) {
        /*
        // под объекты, которые я буду перемещать надо же память выделить?
        // ну типа они же в other, а должны оказаться в this по итогу, я их перемещу -- спасибо, что не скопирую,
        // но место-то под них надо выделить? хотя круто было бы без выделения памяти сказать, что "this, смотри, это твои новые объекты"

        ArrayPtr<Type> temp(other.GetCapacity());
        move(other.begin(), other.end(), &temp[0]);
        simple_v_.swap(temp); */

        // в итоге сделал проще; other не нужны его объекты, раз мы в перемещающем конструкторе, поэтому просто
        swap(other);
    }

    // это нужно если мне целый вектор перемещаемых объектов дадут переместить через SimpleVector<Type> new_vec = moving;
    SimpleVector& operator=(SimpleVector&& rhs) {
        swap(rhs);
        return *this;
    }

    // Обменивает значение с другим вектором
    void swap(SimpleVector& other) noexcept {
        // Напишите тело самостоятельно
        simple_v_.swap(other.simple_v_);
        std::swap(size_, other.size_);
        std::swap(capacity_, other.capacity_);
    }

    // Создаёт вектор из size элементов, инициализированных значением value
    SimpleVector(size_t size, const Type& value = Type{}) {
        // Напишите тело конструктора самостоятельно
        ArrayPtr<Type> tmp(size);
        tmp.swap(simple_v_);

        for (size_t i = 0; i < size; ++i) {
            simple_v_[i] = value;
        }

        size_ = size;
        capacity_ = size;
    }

    // Создаёт вектор из std::initializer_list
    SimpleVector(std::initializer_list<Type> init) {
        // Напишите тело конструктора самостоятельно
        ArrayPtr<Type> tmp(init.size());
        simple_v_.swap(tmp);

        size_t pos = 0;
        for(auto it = init.begin(); it != init.end(); ++it) {
            simple_v_[pos] = *it;
            ++pos;
        }

        size_ = init.size();
        capacity_ = init.size();
    }

    // Добавляет элемент в конец вектора
    // При нехватке места увеличивает вдвое вместимость вектора
    void PushBack(const Type& item) {
        // PushBack это как Insert в конец вектора
        this->Insert(&simple_v_[size_], item);

        // на всякий случай старое пока сохраню

        /* size_t old_size = size_;
        if (size_ == 0) {
            this->Resize(1);
        } else if (size_ >= capacity_) {
            this->Resize(2 * size_);
        }

        size_ = old_size; // ресайз как перевыделитель памяти удобно использовать, но размер надо вернуть какой был
        
        simple_v_[size_] = item;
        ++size_; */
    }

    void PushBack(Type&& item) {
        this->Insert(&simple_v_[size_], std::move(item));

        // на всякий случай старое пока сохраню

        /* size_t old_size = size_;
        if (size_ == 0) {
            this->Resize(1);
        } else if (size_ >= capacity_) {
            this->Resize(2 * size_);
        }
        size_ = old_size; // ресайз как перевыделитель памяти удобно использовать, но размер надо вернуть какой был
        
        simple_v_[size_] = std::move(item);
        ++size_; */
    }

    // "Удаляет" последний элемент вектора. Вектор не должен быть пустым
    // В уроке: Не допускается вызывать PopBack, когда вектор пуст. Такая же особенность есть и у метода pop_back стандартного вектора.
    // В чем состоит это недопущение?
    void PopBack() noexcept {
        // Напишите тело самостоятельно
        if (size_ > 0) {
            --size_;
        }
    }

    // Удаляет элемент вектора в указанной позиции
    Iterator Erase(ConstIterator pos) {
        // Напишите тело самостоятельно
        size_t dist_to_pos = pos - this->begin();

        if (dist_to_pos != size_) {
            std::move(&simple_v_[dist_to_pos + 1], &simple_v_[size_], &simple_v_[dist_to_pos]);
            --size_;
        }

        return &simple_v_[dist_to_pos];
    }

    // Вставляет значение value в позицию pos.
    // Возвращает итератор на вставленное значение
    // Если перед вставкой значения вектор был заполнен полностью,
    // вместимость вектора должна увеличиться вдвое, а для вектора вместимостью 0 стать равной 1
    // (не совсем понятно, как вместимость может быть равна нулю, если мы итератор получили; тогда на что указывает итератор? nullptr что ли?)
    Iterator Insert(ConstIterator pos, const Type& value) {
        // Напишите тело самостоятельно
        size_t dist_to_pos = pos - this->cbegin(); // это потому что pos может в if-е инвалидизироваться
        size_t old_size = size_; // запоминаю по той же причине

        if (size_ == 0) {
            this->Resize(1);
        } else if (size_ >= capacity_) {
            this->Resize(2 * size_);
            std::move_backward(&simple_v_[dist_to_pos], &simple_v_[old_size], &simple_v_[old_size + 1]);
        }

        size_ = old_size; // ресайз сделал размер и вместимость одинаковыми, считая инициализирующие нули существенными,
                          // а мне тут просто вместимость надо увеличить с перевыделением памяти;
                          // но не на 1 же ее увеличивать -- много копирований будет тогда если подряд вставку тестировать
        
        simple_v_[dist_to_pos] = value;
        ++size_;

        return &simple_v_[dist_to_pos];
    }

    Iterator Insert(ConstIterator pos, Type&& value) {
        // Напишите тело самостоятельно
        size_t dist_to_pos = pos - this->cbegin(); // это потому что pos может в if-е инвалидизироваться
        size_t old_size = size_; // запоминаю по той же причине

        if (size_ == 0) {
            this->Resize(1);
        } else if (size_ >= capacity_) {
            this->Resize(2 * size_); // выделять память не ресайзом, т.к. резайз будет копировать
            std::move_backward(&simple_v_[dist_to_pos], &simple_v_[old_size], &simple_v_[old_size + 1]);
        }

        size_ = old_size; // ресайз сделал размер и вместимость одинаковыми, считая инициализирующие нули существенными,
                          // а мне тут просто вместимость надо увеличить с перевыделением памяти;
                          // но не на 1 же ее увеличивать -- много копирований будет тогда если подряд вставку тестировать
        
        simple_v_[dist_to_pos] = std::move(value);
        ++size_;

        return &simple_v_[dist_to_pos];
    }

    // Возвращает количество элементов в массиве
    size_t GetSize() const noexcept {
        // Напишите тело самостоятельно
        return size_;
    }

    // Возвращает вместимость массива
    size_t GetCapacity() const noexcept {
        // Напишите тело самостоятельно
        return capacity_;
    }

    // Сообщает, пустой ли массив
    bool IsEmpty() const noexcept {
        return !size_;
    }

    // Возвращает ссылку на элемент с индексом index
    Type& operator[](size_t index) noexcept {
        // Напишите тело самостоятельно
        return simple_v_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    const Type& operator[](size_t index) const noexcept {
        // Напишите тело самостоятельно
        return simple_v_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    Type& At(size_t index) {
        // Напишите тело самостоятельно
        if (index >= size_) {
            throw std::out_of_range("out of range");
        }

        return simple_v_[index];
    }

    // Возвращает константную ссылку на элемент с индексом index
    // Выбрасывает исключение std::out_of_range, если index >= size
    const Type& At(size_t index) const {
        // Напишите тело самостоятельно
        if (index >= size_) {
            throw std::out_of_range("out of range");
        }

        return simple_v_[index];
    }
    
    // Обнуляет размер массива, не изменяя его вместимость
    void Clear() noexcept {
        // Напишите тело самостоятельно
        size_ = 0;
    }
    
    void Reserve(size_t new_capacity) {
        if (new_capacity > capacity_) {
            size_t old_size = size_;
            // у меня 15 элементов в векторе. я говорю Resize(25) -- мне в итоге надо 15 элементов в векторе и capacity 25;
            // однако у меня в Resize возьмется максимум из двух: max(capacity * 2 = 15 * 2 = 30, new_capacity = 25) = 30;
            // то есть 30 будет capacity, а я загадал 25. и искусственно сейчас буду писать 25;
            // а если старая capacity 10000000, а new_capacity 10000001?
            // тогда у меня в Resize будет выбор из 20000000 и 10000001, capacity станет 20000000. я ее
            // искусственно сделаю 10000001 --> что-то много памяти зря; поэтому я дублирую код -- а как иначе?
            // либо параметр по умолчанию делаю для Resize и исходя из его значения по разным формулам считаю новую capacity
            // наверное это фигово читается. но я выберу так, потому что дублирование кмк еще фиговее
            this->Resize(new_capacity, false);
            size_ = old_size;
        }
    }

    void FillDefault(Iterator start_fill, Iterator end_fill) {
        for (auto it = start_fill; it != end_fill; ++it) {
            *it = Type{};
        }
    }

    // Изменяет размер массива.
    // При увеличении размера новые элементы получают значение по умолчанию для типа Type
    void Resize(size_t new_size, bool how_calc_new_capacity=true) { // ресайз же может пользоваться семантикой перемещения по отношению к тому, что он сейчас копирует
        // Напишите тело самостоятельно

        if (new_size < size_) {
            size_ = new_size;
        } else if (new_size > size_ && new_size < capacity_) {
            FillDefault(this->begin() + size_, this->begin() + new_size);
            size_ = new_size;
        } else if (new_size > capacity_) {

            size_t new_capacity;
            if (how_calc_new_capacity) { // посчитаем новую вместимость изходя из потребнойстей любых методов
                new_capacity = std::max(capacity_ * 2, new_size);
            } else { // и метода Reserve отдельно
                new_capacity = new_size;
            }

            // сделал область видимости для tmp, чтобы при выходе из нее запустился деструктор и удалил его
            // а иначе не понимаю, как еще можно удалить созданный ArrayPtr<Type> tmp(new_capacity);

            {
                ArrayPtr<Type> tmp(new_capacity);
                // std::copy(this->begin(), this->begin() + size_, &tmp[0]); // было :(
                std::move(this->begin(), this->begin() + size_, &tmp[0]); // стало :)
                this->simple_v_.swap(tmp);
            }

                // std::fill( ForwardIt first, ForwardIt last, const T& value ) принимает value по константной ссылке, и поэтому копирует этот аргумент
                // std::fill(start_fill, end_fill, Type{});
                // поэтому пришлось вручную написать заполнение, чтобы вызывался перемещающий оператор присваивания
                FillDefault(this->begin() + size_, this->begin() + new_size);

                size_ = new_size;
                capacity_ = new_capacity;
        }
    }
    
    // Возвращает итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator begin() noexcept {
        // Напишите тело самостоятельно
        return &simple_v_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    Iterator end() noexcept {
        // Напишите тело самостоятельно
        return &simple_v_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator begin() const noexcept {
        // Напишите тело самостоятельно
        return &simple_v_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator end() const noexcept {
        // Напишите тело самостоятельно
        return &simple_v_[size_];
    }

    // Возвращает константный итератор на начало массива
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cbegin() const noexcept {
        // Напишите тело самостоятельно
        return &simple_v_[0];
    }

    // Возвращает итератор на элемент, следующий за последним
    // Для пустого массива может быть равен (или не равен) nullptr
    ConstIterator cend() const noexcept {
        // Напишите тело самостоятельно
        return &simple_v_[size_];
    }

private:
    ArrayPtr<Type> simple_v_;
    size_t size_ = 0;
    size_t capacity_ = 0;
};

// 1) Из урока: "После копирования и заполнения элементов нулевым значением можно обновить size_ и capacity_, а старый массив — удалить."
//    сделал область видимости для new_simple_vector_, чтобы при выходе из нее запустился деструктор и удалил его
//    а иначе не понимаю, как еще можно удалить созданный ArrayPtr<Type> new_simple_vector_(new_capacity); (метод Resize, сторока 221)

template <typename Type>
bool operator==(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return std::equal(lhs.cbegin(), lhs.cend(), rhs.cbegin());
}

template <typename Type>
bool operator!=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(operator==(lhs, rhs));
}

template <typename Type>
bool operator<(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {

    return std::lexicographical_compare(lhs.cbegin(), lhs.cend(), rhs.cbegin(), rhs.cend());
}

template <typename Type>
bool operator<=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
    return !(operator<(rhs, lhs));
}

template <typename Type>
bool operator>(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
// Заглушка. Реализуйте сравнение самостоятельно
    return operator<(rhs, lhs);
}

template <typename Type>
bool operator>=(const SimpleVector<Type>& lhs, const SimpleVector<Type>& rhs) {
// Заглушка. Реализуйте сравнение самостоятельно
    return !(operator<(lhs, rhs));
}
// hello