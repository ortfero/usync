#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"

#include <thread>
#include <usync/usync.hpp>

class resource {
    int value_ {0};

public:
    void turn_up() { ++value_; }
    void turn_down() { --value_; }
    int value() const { return value_; }
};


TEST_CASE("spinlock") {
    resource r;
    usync::spinlock guard;

    auto t1 = std::thread([&]() {
        for(int i = 0; i != 1000; ++i) {
            // access to modify
            std::scoped_lock lock {guard};
            r.turn_up();
        }
    });

    auto t2 = std::thread([&]() {
        for(int i = 0; i != 1000; ++i) {
            // access to modify
            std::scoped_lock lock {guard};
            r.turn_down();
        }
    });

    t1.join();
    t2.join();

    REQUIRE_EQ(r.value(), 0);
}


TEST_CASE("shared_spinlock::shared_spinlock") {
    resource r;
    usync::shared_spinlock guard;

    auto t1 = std::thread([&]() {
        for(int i = 0; i != 1000; ++i) {
            // access to modify
            std::unique_lock lock {guard};
            r.turn_up();
        }
    });

    auto t2 = std::thread([&]() {
        for(int i = 0; i != 1000; ++i) {
            // access to modify
            std::unique_lock lock {guard};
            r.turn_down();
        }
    });

    t1.join();
    t2.join();

    // access to read
    std::shared_lock lock {guard};
    REQUIRE_EQ(r.value(), 0);
}


TEST_CASE("synchronized") {
    using synchronized = usync::synchronized<resource>;

    synchronized resource;

    auto t1 = std::thread([&]() {
        for(int i = 0; i != 1000; ++i) {
            // access to modify
            synchronized::unique_access r {resource};
            r->turn_up();
        }
    });

    auto t2 = std::thread([&]() {
        for(int i = 0; i != 1000; ++i) {
            // access to modify
            synchronized::unique_access r {resource};
            r->turn_down();
        }
    });

    t1.join();
    t2.join();

    // access to read
    synchronized::shared_access r {resource};
    REQUIRE_EQ(r->value(), 0);
}
