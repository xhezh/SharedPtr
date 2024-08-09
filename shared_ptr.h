#pragma once
#define WEAK_PTR_IMPLEMENTED
#include <cstddef>
#include <stdexcept>
#include <algorithm>
#include <utility>

class BadWeakPtr : public std::runtime_error {
public:
  BadWeakPtr() : std::runtime_error("BadWeakPtr") {}
};

struct Counter {
  size_t strong_count;
  size_t weak_count;

  Counter() : strong_count(1), weak_count(0) {}
};

template <typename T>
class WeakPtr;

template <typename T>
class SharedPtr {
private:
  T* ptr_;
  Counter* counter_;

  void Release() {
    if (counter_) {
      if (--(counter_->strong_count) == 0) {
        delete ptr_;
        if (counter_->weak_count == 0) {
          delete counter_;
        }
      }
    }
  }

public:
  SharedPtr() : ptr_(nullptr), counter_(nullptr) {}

  explicit SharedPtr(T* ptr) : ptr_(ptr), counter_(ptr ? new Counter() : nullptr) {}

  SharedPtr(const SharedPtr& other) : ptr_(other.ptr_), counter_(other.counter_) {
    if (counter_) {
      ++(counter_->strong_count);
    }
  }

  SharedPtr(SharedPtr&& other) noexcept : ptr_(other.ptr_), counter_(other.counter_) {
    other.ptr_ = nullptr;
    other.counter_ = nullptr;
  }

  SharedPtr(const WeakPtr<T>& weak_ptr) { // NOLINT
    if (weak_ptr.Expired()) {
      throw BadWeakPtr();
    }
    ptr_ = weak_ptr.ptr_;
    counter_ = weak_ptr.counter_;
    ++(counter_->strong_count);
  }

  SharedPtr& operator=(const WeakPtr<T>& weak_ptr) {
    if (weak_ptr.Expired()) {
      throw BadWeakPtr();
    }
    Release();
    ptr_ = weak_ptr.ptr_;
    counter_ = weak_ptr.counter_;
    ++(counter_->strong_count);
    return *this;
  }

  SharedPtr& operator=(const SharedPtr& other) {
    if (this != &other) {
      Release();
      ptr_ = other.ptr_;
      counter_ = other.counter_;
      if (counter_) {
        ++(counter_->strong_count);
      }
    }
    return *this;
  }

  SharedPtr& operator=(SharedPtr&& other) noexcept {
    if (this != &other) {
      Release();
      ptr_ = other.ptr_;
      counter_ = other.counter_;
      other.ptr_ = nullptr;
      other.counter_ = nullptr;
    }
    return *this;
  }

  ~SharedPtr() {
    Release();
  }

  void Reset(T* ptr = nullptr) {
    if (ptr != ptr_) {
      Release();
      if (ptr) {
        ptr_ = ptr;
        counter_ = new Counter();
      }
      else {
        ptr_ = nullptr;
        counter_ = nullptr;
      }
    }
  }

  void Swap(SharedPtr& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(counter_, other.counter_);
  }

  T* Get() const {
    return ptr_;
  }

  size_t UseCount() const {
    return counter_ ? counter_->strong_count : 0;
  }

  T& operator*() const {
    return *ptr_;
  }

  T* operator->() const {
    return ptr_;
  }

  explicit operator bool() const {
    return ptr_ != nullptr;
  }

  friend class WeakPtr<T>;
};

template <typename T>
class WeakPtr {
private:
  T* ptr_;
  Counter* counter_;

  void Release() {
    if (counter_) {
      if (--(counter_->weak_count) == 0 && counter_->strong_count == 0) {
        delete counter_;
      }
    }
  }

public:
  WeakPtr() : ptr_(nullptr), counter_(nullptr) {}

  WeakPtr(const WeakPtr& other) : ptr_(other.ptr_), counter_(other.counter_) {
    if (counter_) {
      ++(counter_->weak_count);
    }
  }

  WeakPtr(WeakPtr&& other) noexcept : ptr_(other.ptr_), counter_(other.counter_) {
    other.ptr_ = nullptr;
    other.counter_ = nullptr;
  }

  WeakPtr(const SharedPtr<T>& shared_ptr) : ptr_(shared_ptr.ptr_), counter_(shared_ptr.counter_) { // NOLINT
    if (counter_) {
      ++(counter_->weak_count);
    }
  }

  WeakPtr& operator=(const SharedPtr<T>& shared_ptr) {
    Release();
    ptr_ = shared_ptr.ptr_;
    counter_ = shared_ptr.counter_;
    if (counter_) {
      ++(counter_->weak_count);
    }
    return *this;
  }

  WeakPtr& operator=(const WeakPtr& other) {
    if (this != &other) {
      Release();
      ptr_ = other.ptr_;
      counter_ = other.counter_;
      if (counter_) {
        ++(counter_->weak_count);
      }
    }
    return *this;
  }

  WeakPtr& operator=(WeakPtr&& other) noexcept {
    if (this != &other) {
      Release();
      ptr_ = other.ptr_;
      counter_ = other.counter_;
      other.ptr_ = nullptr;
      other.counter_ = nullptr;
    }
    return *this;
  }

  ~WeakPtr() {
    Release();
  }

  void Reset() {
    Release();
    ptr_ = nullptr;
    counter_ = nullptr;
  }

  void Swap(WeakPtr& other) noexcept {
    std::swap(ptr_, other.ptr_);
    std::swap(counter_, other.counter_);
  }

  size_t UseCount() const {
    return counter_ ? counter_->strong_count : 0;
  }

  bool Expired() const {
    return UseCount() == 0;
  }

  SharedPtr<T> Lock() const {
    if (Expired()) {
      return SharedPtr<T>();
    }
    return SharedPtr<T>(*this);
  }

  friend class SharedPtr<T>;
};

template <typename T, typename... Args>
SharedPtr<T> MakeShared(Args&&... args) {
  return SharedPtr<T>(new T(std::forward<Args>(args)...));
}
