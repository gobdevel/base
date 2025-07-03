/*
 * @file singleton.h
 * @brief Thread-safe singleton implementations for C++11 and later
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#pragma once

#include <memory>
#include <utility>

namespace base {

/**
 * @brief Static singleton factory - provides thread-safe access to singleton instances.
 *
 * This class provides a static interface for creating and accessing singleton instances
 * of any type T. The instance is created on first access using Meyer's singleton pattern,
 * which is thread-safe in C++11 and later.
 *
 * Usage:
 * @code
 * auto& logger = Singleton<Logger>::instance();
 * auto& config = Singleton<Config>::instance();
 * @endcode
 *
 * @tparam T The type to create a singleton of
 * @note Thread-safe initialization guaranteed by C++11 static local initialization
 * @note T must be default constructible
 */
template <typename T>
class Singleton {
public:
    // Delete all constructors and assignment operators
    Singleton() = delete;
    Singleton(const Singleton&) = delete;
    Singleton& operator=(const Singleton&) = delete;
    Singleton(Singleton&&) = delete;
    Singleton& operator=(Singleton&&) = delete;

    /**
     * @brief Get the singleton instance of type T.
     * @return Reference to the singleton instance
     * @note Thread-safe lazy initialization
     */
    static T& instance() {
        static T instance{};
        return instance;
    }

    /**
     * @brief Get a shared_ptr to the singleton instance.
     * @return Shared pointer to the singleton instance
     * @note Useful when you need shared ownership semantics
     */
    static std::shared_ptr<T> shared_instance() {
        return std::shared_ptr<T>(&instance(), [](T*) {
            // Custom deleter that does nothing - singleton manages its own lifetime
        });
    }
};

/**
 * @brief CRTP base class for implementing singleton pattern via inheritance.
 *
 * This class uses the Curiously Recurring Template Pattern (CRTP) to provide
 * singleton functionality to derived classes. Derived classes automatically
 * get singleton behavior by inheriting from this base.
 *
 * Usage:
 * @code
 * class MyClass : public SingletonBase<MyClass> {
 *     friend class SingletonBase<MyClass>; // Allow base to construct
 * public:
 *     void doSomething() { ... }
 * private:
 *     MyClass() = default; // Private constructor
 * };
 *
 * auto& instance = MyClass::instance();
 * @endcode
 *
 * @tparam Derived The derived class type
 * @note Derived class should declare SingletonBase as friend
 * @note Derived class constructor should be private/protected
 */
template <typename Derived>
class SingletonBase {
public:
    /**
     * @brief Get the singleton instance of the derived class.
     * @return Reference to the singleton instance
     * @note Thread-safe lazy initialization
     */
    static Derived& instance() {
        static Derived instance{};
        return instance;
    }

    /**
     * @brief Get a shared_ptr to the singleton instance.
     * @return Shared pointer to the singleton instance
     */
    static std::shared_ptr<Derived> shared_instance() {
        return std::shared_ptr<Derived>(&instance(), [](Derived*) {
            // Custom deleter that does nothing
        });
    }

    // Delete copy and move operations
    SingletonBase(const SingletonBase&) = delete;
    SingletonBase& operator=(const SingletonBase&) = delete;
    SingletonBase(SingletonBase&&) = delete;
    SingletonBase& operator=(SingletonBase&&) = delete;

protected:
    // Protected constructor - only derived classes can construct
    SingletonBase() = default;
    virtual ~SingletonBase() = default;
};

} // namespace base
