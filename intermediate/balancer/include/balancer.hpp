#ifndef BALANCER_H
#define BALANCER_H

#include <unistd.h>
#include <fcntl.h>
#include <csignal>

#include <iostream>
#include <string>
#include <list>
#include <unordered_map>
#include <tuple>
#include <filesystem>

#include "nlohmann/json.hpp"

namespace mrpc {

namespace fs = std::filesystem;
const static std::int32_t INITIAL_WEIGHT = 5;
const static std::int32_t INITIAL_THRESHOLD = 20;

int balancerConfigure(const std::string&);

class Service {
private:
    std::string url{};
    std::int16_t port{};
    std::int32_t weight{};

public:
    Service() = default;

    Service( std::string  url_,
            std::int16_t port_,
            std::int32_t weight_) : url(std::move(url_)), port(port_), weight(weight_) {};

    ~Service() = default;

    bool isDown() const;

    std::string getUrl() const;

    std::int16_t getPort() const;

    std::int32_t getWeight() const;

    void updateWeight();

    void reset();
};

class Balancer {
private:
    std::unordered_map<std::string, std::list<Service>> hosts{};
    std::unordered_map<std::string, std::int32_t> threshold{};
    ::pid_t registration_pid{};

    void requestData(const std::string&, ::pid_t, const fs::path&);
public:
    Balancer() = default;

    static Balancer& instance() {
        static Balancer balancer{};
        return balancer;
    }

    void insertEntry(const std::string&, std::string, std::int16_t);

    void initialize(::pid_t, const fs::path&);

    std::tuple<std::string, std::int16_t> epoll(const std::string&);

    void resetWeights(const std::string&);

    void updateHosts(const std::string&, ::pid_t, const fs::path&);
};
}// namespace mrpc

#endif //BALANCER_H
