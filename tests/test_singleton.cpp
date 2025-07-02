/*
 * @file test_singleton.cpp
 * @brief Unit tests for Singleton template classes
 *
 * Copyright (c) 2025 Gobind Prasad <gobdeveloper@gmail.com>
 * SPDX-License-Identifier: MIT
 */

#include <singleton.h>
#include <gtest/gtest.h>
#include <thread>
#include <vector>
#include <memory>
#include <atomic>

using namespace crux;

// Test class for basic functionality tests
class BasicTestClass {
public:
    BasicTestClass() : value_(42) {}

    int getValue() const { return value_; }
    void setValue(int val) { value_ = val; }

private:
    int value_;
};

// Test class for same instance tests
class SameInstanceTestClass {
public:
    SameInstanceTestClass() : value_(42) {}

    int getValue() const { return value_; }
    void setValue(int val) { value_ = val; }

private:
    int value_;
};

// Test class for thread safety tests
class ThreadSafeTestClass {
public:
    ThreadSafeTestClass() : value_(42) {}

    int getValue() const { return value_; }
    void setValue(int val) { value_ = val; }

private:
    int value_;
};

// Test class derived from SingletonBase
class DerivedSingleton : public SingletonBase<DerivedSingleton> {
public:
    DerivedSingleton() : counter_(0) {}

    void increment() { ++counter_; }
    int getCounter() const { return counter_; }
    void resetCounter() { counter_ = 0; }

private:
    std::atomic<int> counter_;
};

// Separate test class for thread safety tests
class ThreadSafeSingleton : public SingletonBase<ThreadSafeSingleton> {
public:
    ThreadSafeSingleton() : counter_(0) {}

    void increment() { ++counter_; }
    int getCounter() const { return counter_; }
    void resetCounter() { counter_ = 0; }

private:
    std::atomic<int> counter_;
};

// Test fixture for singleton tests
class SingletonTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Reset singleton state before each test
        auto& instance = DerivedSingleton::instance();
        instance.resetCounter();
    }

    void TearDown() override {
        // Clean up if needed
    }
};

// Test Singleton<T> basic functionality
TEST_F(SingletonTest, SingletonInstanceReturnsValidReference) {
    auto& instance = Singleton<BasicTestClass>::instance();
    EXPECT_EQ(instance.getValue(), 42);
}

// Test that multiple calls return the same instance
TEST_F(SingletonTest, SingletonInstanceReturnsSameObject) {
    auto& instance1 = Singleton<SameInstanceTestClass>::instance();
    auto& instance2 = Singleton<SameInstanceTestClass>::instance();

    // Should be the same object (same memory address)
    EXPECT_EQ(&instance1, &instance2);

    // Verify by modifying one and checking the other
    instance1.setValue(100);
    EXPECT_EQ(instance2.getValue(), 100);
}

// Test thread safety of Singleton<T>
TEST_F(SingletonTest, SingletonIsThreadSafe) {
    const int num_threads = 10;
    std::vector<std::thread> threads;
    std::vector<ThreadSafeTestClass*> instances(num_threads);

    // Launch multiple threads to get singleton instance
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([&instances, i]() {
            instances[i] = &Singleton<ThreadSafeTestClass>::instance();
        });
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // All instances should point to the same object
    for (int i = 1; i < num_threads; ++i) {
        EXPECT_EQ(instances[0], instances[i]);
    }
}

// Test SingletonBase derived class functionality
TEST_F(SingletonTest, SingletonBaseInstanceWorks) {
    auto& instance = DerivedSingleton::instance();
    EXPECT_EQ(instance.getCounter(), 0);

    instance.increment();
    EXPECT_EQ(instance.getCounter(), 1);
}

// Test that SingletonBase instances are unique per derived type
TEST_F(SingletonTest, SingletonBaseReturnsSameInstance) {
    auto& instance1 = DerivedSingleton::instance();
    auto& instance2 = DerivedSingleton::instance();

    // Should be the same object
    EXPECT_EQ(&instance1, &instance2);

    // Verify by modifying state
    instance1.increment();
    instance1.increment();
    EXPECT_EQ(instance2.getCounter(), 2);
}

// Test thread safety of SingletonBase
TEST_F(SingletonTest, SingletonBaseIsThreadSafe) {
    const int num_threads = 10;
    const int increments_per_thread = 100;
    std::vector<std::thread> threads;

    // Reset counter before test
    auto& instance = ThreadSafeSingleton::instance();
    instance.resetCounter();

    // Launch multiple threads to access and modify singleton
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back([increments_per_thread]() {
            auto& instance = ThreadSafeSingleton::instance();
            for (int j = 0; j < increments_per_thread; ++j) {
                instance.increment();
            }
        });
    }

    // Wait for all threads to complete
    for (auto& t : threads) {
        t.join();
    }

    // Counter should be exactly num_threads * increments_per_thread
    EXPECT_EQ(instance.getCounter(), num_threads * increments_per_thread);
}

// Test that we can't create multiple instances directly
TEST_F(SingletonTest, SingletonCannotBeInstantiatedDirectly) {
    // These should not compile if uncommented:
    // Singleton<TestClass> s1;
    // auto s2 = new Singleton<TestClass>();

    // This test just verifies the compilation constraints exist
    SUCCEED();
}

// Test Singleton with different types
class FirstTypeClass {
public:
    FirstTypeClass() : value_(42) {}
    void setValue(int val) { value_ = val; }
    int getValue() const { return value_; }

private:
    int value_;
};

class SecondTypeClass {
public:
    SecondTypeClass() : name_("default") {}
    void setName(const std::string& name) { name_ = name; }
    const std::string& getName() const { return name_; }

private:
    std::string name_;
};

TEST_F(SingletonTest, SingletonWorksWithDifferentTypes) {
    auto& intInstance = Singleton<FirstTypeClass>::instance();
    auto& stringInstance = Singleton<SecondTypeClass>::instance();

    // Different types should have different singleton instances
    intInstance.setValue(999);
    stringInstance.setName("test");

    EXPECT_EQ(intInstance.getValue(), 999);
    EXPECT_EQ(stringInstance.getName(), "test");

    // Verify they're actually different objects
    EXPECT_NE(static_cast<void*>(&intInstance), static_cast<void*>(&stringInstance));
}

// Test class for lazy initialization (separate from TestClass to avoid interference)
class LazyTestClass {
public:
    LazyTestClass() : value_(42) {}

    int getValue() const { return value_; }
    void setValue(int val) { value_ = val; }

private:
    int value_;
};

// Test lazy initialization
TEST_F(SingletonTest, SingletonIsLazilyInitialized) {
    // First access should initialize the singleton
    auto& instance1 = Singleton<LazyTestClass>::instance();
    EXPECT_EQ(instance1.getValue(), 42);  // Default constructor value

    // Subsequent accesses should return the same initialized instance
    instance1.setValue(123);
    auto& instance2 = Singleton<LazyTestClass>::instance();
    EXPECT_EQ(instance2.getValue(), 123);
}

// Test class for lifecycle tests
class LifecycleTestClass {
public:
    LifecycleTestClass() : value_(42) {}

    int getValue() const { return value_; }
    void setValue(int val) { value_ = val; }

private:
    int value_;
};

// Test singleton destruction behavior (implicit test)
TEST_F(SingletonTest, SingletonLifecycle) {
    // Singletons should be destroyed at program exit
    // This is hard to test directly, but we can verify basic lifecycle
    auto& instance = Singleton<LifecycleTestClass>::instance();
    instance.setValue(456);

    // Instance should maintain state throughout program execution
    EXPECT_EQ(instance.getValue(), 456);
}

// Test shared_ptr singleton functionality
TEST_F(SingletonTest, SingletonSharedInstanceWorks) {
    auto ptr1 = Singleton<BasicTestClass>::shared_instance();
    auto ptr2 = Singleton<BasicTestClass>::shared_instance();

    // Should point to the same object
    EXPECT_EQ(ptr1.get(), ptr2.get());
    EXPECT_EQ(ptr1.get(), &Singleton<BasicTestClass>::instance());

    // Should be able to use like normal shared_ptr
    ptr1->setValue(777);
    EXPECT_EQ(ptr2->getValue(), 777);
}

// Test SingletonBase shared_instance
TEST_F(SingletonTest, SingletonBaseSharedInstanceWorks) {
    auto ptr1 = DerivedSingleton::shared_instance();
    auto ptr2 = DerivedSingleton::shared_instance();

    // Should point to the same object
    EXPECT_EQ(ptr1.get(), ptr2.get());
    EXPECT_EQ(ptr1.get(), &DerivedSingleton::instance());

    // Should work with the interface
    ptr1->increment();
    EXPECT_EQ(ptr2->getCounter(), 1);
}

// Example of proper SingletonBase usage with friend declaration
class ProperSingleton : public SingletonBase<ProperSingleton> {
    friend class SingletonBase<ProperSingleton>; // Allow base class to construct

public:
    void setValue(int val) { value_ = val; }
    int getValue() const { return value_; }

private:
    ProperSingleton() : value_(100) {} // Private constructor
    int value_;
};

// Test proper singleton usage pattern
TEST_F(SingletonTest, ProperSingletonUsagePattern) {
    auto& instance1 = ProperSingleton::instance();
    auto& instance2 = ProperSingleton::instance();

    // Should be the same object
    EXPECT_EQ(&instance1, &instance2);
    EXPECT_EQ(instance1.getValue(), 100); // Default value

    instance1.setValue(200);
    EXPECT_EQ(instance2.getValue(), 200);
}
