#include "include/balancer.hpp"

bool mrpc::Service::isDown() const {
    return weight == 0;
}

std::string mrpc::Service::getUrl() const {
    return url;
}

std::int16_t mrpc::Service::getPort() const {
    return port;
}

void mrpc::Service::updateWeight() {
    weight--;
}

void mrpc::Service::reset() {
    weight = INITIAL_WEIGHT;
}

std::int32_t mrpc::Service::getWeight() const {
    return weight;
}

std::tuple<std::string, std::int16_t> mrpc::Balancer::epoll(const std::string& func_name) {
    auto& host_list = hosts.at(func_name);

    auto top = host_list.begin();
    top->updateWeight();

    auto dest = std::find_if(++host_list.cbegin(), host_list.cend(),
                             [top](Service& s) { return s.getWeight() < top->getWeight(); });
    host_list.splice(dest, host_list, top);

    return std::make_tuple(top->getUrl(), top->getPort());
}

void mrpc::Balancer::initialize(::pid_t pid, const fs::path& path) {
    registration_pid = pid;
    fifo_path = path;
    requestData("ALL");
}

void mrpc::Balancer::insertEntry(const std::string& func_name, std::string ip, std::int16_t port) {
    hosts[func_name].emplace_back(std::move(ip), port, INITIAL_WEIGHT);
    threshold[func_name] = mrpc::INITIAL_THRESHOLD;
}

void mrpc::Balancer::resetWeights(const std::string& func_name) {
    for (auto& service: hosts[func_name]) {
        service.reset();
    }
    threshold[func_name]--;

    if (!threshold[func_name]) {
        requestData(func_name);
    }
}

void
mrpc::Balancer::updateHosts(const std::string& func_name) {
    if (threshold[func_name] > 0) {
        threshold[func_name]--;
    } else {
        requestData(func_name);
    }
}

void
mrpc::Balancer::requestData(const std::string& func_name) {
    ::kill(registration_pid, SIGUSR1);
    int fifo_fd;

    if ((fifo_fd = ::open(fifo_path.c_str(), O_WRONLY)) < 0) {
        std::cerr << "Failed open FIFO" << std::endl;
        std::cerr << "errno: " << errno << std::endl;
    }
    ::write(fifo_fd, func_name.c_str(), func_name.size());
    ::close(fifo_fd);

    // block until write open
    if ((fifo_fd = ::open(fifo_path.c_str(), O_RDONLY)) < 0) {
        std::cerr << "Failed open FIFO" << std::endl;
        std::cerr << "errno: " << errno << std::endl;
    }

    /*
     * json format
     * [
     * {
     *      "func_name": "",
     *      "ip": "",
     *      "port": 0
     * },
     * ...
     * ]
     */
    std::string data{};
    char buf[1024];
    ssize_t read_size;
    while ((read_size = ::read(fifo_fd, (void*) buf, 1024)) > 0) {
        data.append(buf);
    }

    nlohmann::json json = nlohmann::json::parse(data);
    for (auto& element: json) {
        insertEntry(json.at("func_name").get<std::string>(),
                    json.at("ip").get<std::string>(),
                    json.at("port").get<int16_t>());
    }
}

void mrpc::Balancer::setRegistrationPid(::pid_t pid) {
    registration_pid = pid;
}
