#ifndef BALANCER_H
#define BALANCER_H

#include <unistd.h>

#include <string>
#include <list>
#include <unordered_map>

namespace mrpc {

const std::int32_t INITIAL_WEIGHT = 5;

class Service {
private:
    std::string serviceName;
    std::string url;
    std::int16_t port;
    std::int32_t weight;

public:
    Service() = default;

    Service(const std::string& serviceName_,
            const std::string& url_,
            std::int16_t port_,
            std::int32_t weight_) : serviceName(serviceName_), url(url_),
                          port(port_), weight(weight_) {};

    ~Service() = default;

    bool isDown();

    std::string getServiceName();

    std::string getUrl();

    std::int16_t getPort();

    std::int32_t getWeight();

    void updateWeight();

    void clear();
};

class Balancer {
private:
    std::unordered_map<std::string, std::list<Service>> hosts;
    ::pid_t registration_pid;
public:
    Balancer() = default;

    static Balancer& instance() {
        static Balancer balancer{};
        return balancer;
    }

    void configure(::pid_t);

    void epoll(const std::string&);

    bool clearWeights(const std::string&);

    void updateHosts(const std::string&);
};
}// namespace mrpc

#endif //BALANCER_H
