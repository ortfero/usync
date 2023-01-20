# usync
C++17 header-only library for spinlocks

## Snippets

### Using synchronized access

```cpp
#include <cassert>
#include <thread>
#include <usync/usync.hpp>


class resource {
    int value_{0};
public:
    void turn_up() { ++value_; }
    void turn_down() { --value_; }
    int value() const { return value_; }
};


int main() {
    using synchronized = usync::synchronized<resource>;
    
    synchronized resource;
    
    auto t1 = std::thread([&]() {
        for(int i = 0; i != 1000; ++i) {
            // access to modify
            synchronized::unique_access r{ resource };
            r->turn_up();
        }
    });
    
    auto t2 = std::thread([&]() {
        for(int i = 0; i != 1000; ++i) {
            // access to modify
            synchronized::unique_access r{ resource };
            r->turn_down();
        }
    });
    
    t1.join();
    t2.join();
    
    // access to read
    synchronized::shared_access r{ resource };
    assert(r->value() == 0);
}
```

### Specify lock policy for synchronized access

```cpp
using sync_with_spinlock = usync::synchronized<resource>;
using sync_with_shared_spinlock = usync::synchronized<resource, usync::shared_spinlock>;
using sync_with_no_lock = usync::synchronized<resource, usync::no_lock>;
using sync_with_mutex = usync::synchronized<resource, std::mutex>;
```

### Using spinlock

```cpp
#include <mutex>
#include <thread>
#include <usync/usync.hpp>

int main() {
    int resource = 0;
    usync::spinlock lock;
    
    auto t = std::thread([&]() {
        std::scoped_lock guard(lock);
        resource = -1;
    });
    
    t.join();
    return 0;
}
```

### Using shared_spinlock

```cpp
#include <mutex>
#include <thread>
#include <cstdio>
#include <usync/usync.hpp>

int main() {
    int resource = 0;
    usync::shared_spinlock lock;
    
    auto t1 = std::thread([&]() {
        // access to modify
        std::unique_lock guard(lock);
        resource = -1;
    });
    
    auto t2 = std::thread([&]() {
        // access to read
        std::shared_lock guard(lock);
        std::printf("%d\n", resource);
    });
    
    t1.join();
    t2.join();
    return 0;
}
```



