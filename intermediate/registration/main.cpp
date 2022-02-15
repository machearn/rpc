#include <unistd.h>
#include <fcntl.h>

#include <filesystem>

#include "nlohmann/json.hpp"
#include "include/register.hpp"
#include "signal.hpp"

std::filesystem::path fifo_path{};

void sigusr1(int signo) {
    int fifo_fd;
    if ((fifo_fd = ::open(fifo_path.c_str(), O_RDONLY)) < 0) {
        std::cerr << "Failed open FIFO" << std::endl;
        std::cerr << "errno: " << errno << std::endl;
    }
    std::string func_name{};
    char buf[1024];
    while (::read(fifo_fd, buf, 1024) > 0) {
        func_name.append(buf);
    }
    ::close(fifo_fd);

    auto result = mrpc::Register::instance().query(func_name);
    nlohmann::json json;
    for (auto& element: result) {
        json.push_back({
                               {"func_name", func_name},
                               {"ip", std::get<0>(element)},
                               {"port", std::get<1>(element)}
                       });
    }
    std::string json_str = json.dump();

    if ((fifo_fd = ::open(fifo_path.c_str(), O_RDONLY)) < 0) {
        std::cerr << "Failed open FIFO" << std::endl;
        std::cerr << "errno: " << errno << std::endl;
    }

    ::write(fifo_fd, json_str.c_str(), json_str.size());
    ::close(fifo_fd);
}

int main(int argc, char** argv) {
    if (mrpc::signal(SIGUSR1, sigusr1) == SIG_ERR) {
        std::cerr << "set signal error" << std::endl;
        return -1;
    }
    mrpc::configure(std::move(mongocxx::uri{}));
    
    for( ; ;) {
        ::pause();
    }
    return 0;
}