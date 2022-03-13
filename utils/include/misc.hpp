#ifndef RPC_MISC_HPP
#define RPC_MISC_HPP

namespace mrpc {
enum class OP {
    INSERT,
    QUERY,
    UPDATE,
    DROP,
};

class Counter {
public:
    Counter(): count_(mrpc::REGISTER_THRESHOLD), mutex_() {}
    ~Counter()=default;

    void reset() {
        count_ = mrpc::REGISTER_THRESHOLD;
    }

    void decrement() {
        if (count_ > 0)
            count_--;
    }

    void increment() {
        count_++;
    }

    [[nodiscard]] std::int32_t get() const {
        return count_;
    }

    void lock() {
        while (mutex_);
        mutex_ = 1;
    }

    void unlock() {
        mutex_ = 0;
    }

    bool try_lock() {
        if (mutex_) return false;
        else {
            mutex_ = 1;
            return true;
        }
    }
private:
    std::int32_t count_;
    std::int32_t mutex_;
};

struct Message {
    long type;
    ::pid_t pid;
    char buffer[512];

    Message(): type(), pid(), buffer() {}
    ~Message()=default;
};
} //namespace mrpc

#endif //RPC_MISC_HPP
