#pragma once

#include <cassert>
#include <cstdlib>
#include <memory>
#include <new>
#include <utility>

template <typename T>
class RawMemory {
public:
    RawMemory() = default;

    explicit RawMemory(size_t capacity)
        : buffer_(Allocate(capacity))
        , capacity_(capacity) {
    }

    ~RawMemory() {
        Deallocate(buffer_);
    }

    RawMemory(const RawMemory&) = delete;

    RawMemory& operator=(const RawMemory& rhs) = delete;

    RawMemory(RawMemory&& other) noexcept {
        Swap(other);
    }

    RawMemory& operator=(RawMemory&& rhs) noexcept {
        Swap(rhs);
        return *this;
    }

    T* operator+(size_t offset) noexcept {
        assert(offset <= capacity_);
        return buffer_ + offset;
    }

    const T* operator+(size_t offset) const noexcept {
        return const_cast<RawMemory&>(*this) + offset;
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<RawMemory&>(*this)[index];
    }

    T& operator[](size_t index) noexcept {
        assert(index < capacity_);
        return buffer_[index];
    }

    void Swap(RawMemory& other) noexcept {
        std::swap(buffer_, other.buffer_);
        std::swap(capacity_, other.capacity_);
    }

    const T* GetAddress() const noexcept {
        return buffer_;
    }

    T* GetAddress() noexcept {
        return buffer_;
    }

    size_t Capacity() const {
        return capacity_;
    }

private:
    static T* Allocate(size_t n) {
        return n != 0 ? static_cast<T*>(operator new(n * sizeof(T))) : nullptr;
    }

    static void Deallocate(T* buf) noexcept {
        operator delete(buf);
    }

    T* buffer_ = nullptr;
    size_t capacity_ = 0;
};

template <typename T>
class Vector {
public:
    using iterator = T*;
    using const_iterator = const T*;

    Vector() = default;

    explicit Vector(size_t size)
        : data_(size)
        , size_(size)
    {
        std::uninitialized_value_construct_n(begin(), size);
    }

    Vector(const Vector& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        std::uninitialized_copy_n(other.begin(), other.size_, begin());
    }

    Vector& operator=(const Vector& rhs) {
        if (this != &rhs) {
            if (rhs.size_ > data_.Capacity()) {
                Vector rhs_copy(rhs);
                Swap(rhs_copy);
            } else {
                if (rhs.size_ < size_) {
                    for (size_t i = 0; i < rhs.size_; ++i) {
                        data_[i] = rhs.data_[i];
                    }
                    std::destroy_n(begin() + rhs.size_, size_ - rhs.size_);
                } else {
                    for (size_t i = 0; i < size_; ++i) {
                        data_[i] = rhs.data_[i];
                    }
                    std::uninitialized_copy_n(rhs.begin() + size_, rhs.size_ - size_, begin() + size_);
                }
                size_ = rhs.size_;
            }
        }
        return *this;
    }

    Vector(Vector&& other)
        : data_(other.size_)
        , size_(other.size_)
    {
        data_.Swap(other.data_);
        other.size_ = 0;
    }
    Vector& operator=(Vector&& rhs) {
        data_.Swap(rhs.data_);
        rhs.size_ = 0;
        return *this;
    }

    ~Vector() {
        std::destroy_n(begin(), size_);
    }

    iterator begin() noexcept {
        return data_.GetAddress();
    }

    iterator end() noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator begin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator end() const noexcept {
        return data_.GetAddress() + size_;
    }

    const_iterator cbegin() const noexcept {
        return data_.GetAddress();
    }

    const_iterator cend() const noexcept {
        return data_.GetAddress() + size_;
    }

    void Swap(Vector& other) noexcept {
        data_.Swap(other.data_);
        std::swap(size_, other.size_);
    }

    size_t Size() const noexcept {
        return size_;
    }

    size_t Capacity() const noexcept {
        return data_.Capacity();
    }

    T& operator[](size_t index) noexcept {
        assert(index < size_);
        return data_[index];
    }

    const T& operator[](size_t index) const noexcept {
        return const_cast<Vector&>(*this)[index];
    }

    void Reserve(size_t new_capacity) {
        if (new_capacity <= data_.Capacity()) {
            return;
        }
        RawMemory<T> new_data(new_capacity);
        if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
            std::uninitialized_move_n(begin(), size_, new_data.GetAddress());
        } else {
            std::uninitialized_copy_n(begin(), size_, new_data.GetAddress());
        }
        std::destroy_n(begin(), size_);
        data_.Swap(new_data);
    }

    void Resize(size_t new_size) {
        Reserve(new_size);
        if (size_ < new_size) {
            std::uninitialized_value_construct_n(begin() + size_, new_size - size_);
        } else if (new_size < size_) {
            std::destroy_n(begin() + new_size, size_ - new_size);
        }
        size_ = new_size;
    }

    template <typename... Args>
    T& EmplaceBack(Args&&... args) {
        if (size_ == data_.Capacity()) {
            RawMemory<T> new_data((size_ == 0) ? 1 : 2 * size_);
            new (new_data.GetAddress() + size_) T(std::forward<Args>(args)...);
            if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                std::uninitialized_move_n(begin(), size_, new_data.GetAddress());
            } else {
                std::uninitialized_copy_n(begin(), size_, new_data.GetAddress());
            }
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        } else {
            new (begin() + size_) T(std::forward<Args>(args)...);
        }
        ++size_;
        return *(begin() + size_ - 1);
    }

    void PushBack(const T& value) {
        EmplaceBack(value);
    }

    void PushBack(T&& value) {
        EmplaceBack(std::move(value));
    }

    template <typename... Args>
    iterator Emplace(const_iterator position, Args&&... args) {
        size_t pos = position - begin();
        if (size_ == data_.Capacity()) {
            size_t count = end() - position;
            RawMemory<T> new_data((size_ == 0) ? 1 : 2 * size_);
            new (new_data.GetAddress() + pos) T(std::forward<Args>(args)...);
            try {
                if constexpr (std::is_nothrow_move_constructible_v<T> || !std::is_copy_constructible_v<T>) {
                    std::uninitialized_move_n(begin(), pos, new_data.GetAddress());
                    std::uninitialized_move_n(begin() + pos, count, new_data.GetAddress() + pos + 1);
                } else {
                    std::uninitialized_copy_n(begin(), pos, new_data.GetAddress());
                    std::uninitialized_copy_n(begin() + pos, count, new_data.GetAddress() + pos + 1);
                }
            } catch (...) {
                new_data.~RawMemory();
            }
            std::destroy_n(begin(), size_);
            data_.Swap(new_data);
        } else {
            if (size_ != pos) {
                new (end()) T(std::forward<T>(data_[size_ - 1]));
                std::move_backward(begin() + pos, end() - 1, end());
                data_[pos] = T(std::forward<Args>(args)...);
            } else {
                new (begin() + pos) T(std::forward<Args>(args)...);
            }
        }
        ++size_;
        return begin() + pos;
    }

    iterator Insert(const_iterator position, const T& value) {
        return Emplace(position, value);
    }

    iterator Insert(const_iterator position, T&& value) {
        return Emplace(position, std::move(value));
    }

    void PopBack() noexcept {
        assert(size_ > 0);
        size_--;
        data_[size_].~T();
    }

    iterator Erase(const_iterator position) noexcept(std::is_nothrow_move_assignable_v<T>) {
        size_t pos = position - begin();
        std::move(begin() + pos + 1, end(), begin() + pos);
        std::destroy_n(end() - 1, 1);
        --size_;
        return begin() + pos;
    }

private:
    RawMemory<T> data_;
    size_t size_ = 0;
};