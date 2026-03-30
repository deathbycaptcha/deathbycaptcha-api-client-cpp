// example.ThreadSafe_Http.cpp — one HttpClient instance per thread
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <vector>

#include "deathbycaptcha/deathbycaptcha.hpp"

namespace {
std::string getenv_or_empty(const char* key) {
    const char* v = std::getenv(key);
    return v ? std::string(v) : std::string{};
}

void run_http_worker(const std::string& username,
                     const std::string& password,
                     const std::string& image_path,
                     bool decode_mode,
                     int worker_id) {
    // Each worker owns its own client instance.
    dbc::HttpClient client(username, password);
    client.is_verbose = false;

    try {
        if (!decode_mode) {
            std::cout << "[HTTP worker " << worker_id
                      << "] balance: " << client.get_balance() << " US cents\n";
        } else {
            auto result = client.decode(image_path, 120);
            if (result && result->text.has_value()) {
                std::cout << "[HTTP worker " << worker_id << "] solved captcha "
                          << result->captcha << ": " << *result->text << '\n';
            } else {
                std::cout << "[HTTP worker " << worker_id
                          << "] timeout/no result\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "[HTTP worker " << worker_id << "] error: " << e.what()
                  << '\n';
    }
}
} // namespace

int main() {
    const std::string username = getenv_or_empty("DBC_USERNAME");
    const std::string password = getenv_or_empty("DBC_PASSWORD");
    const std::string image_path = getenv_or_empty("DBC_IMAGE_PATH");
    const std::string threads_env = getenv_or_empty("DBC_THREADS");

    if (username.empty() || password.empty()) {
        std::cerr << "Set DBC_USERNAME and DBC_PASSWORD before running.\n";
        return 1;
    }

    int threads_count = 2;
    if (!threads_env.empty()) {
        try {
            threads_count = std::stoi(threads_env);
        } catch (...) {
            std::cerr << "Invalid DBC_THREADS value. Use a positive integer.\n";
            return 1;
        }
    }
    if (threads_count <= 0) {
        std::cerr << "DBC_THREADS must be greater than zero.\n";
        return 1;
    }

    const bool decode_mode = !image_path.empty();

    // Main thread sanity check in normal usage.
    try {
        dbc::HttpClient sanity_client(username, password);
        sanity_client.is_verbose = false;
        std::cout << "[HTTP main] initial balance: "
                  << sanity_client.get_balance() << " US cents\n";
    } catch (const std::exception& e) {
        std::cerr << "[HTTP main] error: " << e.what() << '\n';
        return 1;
    }

    std::vector<std::thread> workers;
    workers.reserve(static_cast<std::size_t>(threads_count));

    for (int i = 0; i < threads_count; ++i) {
        workers.emplace_back(run_http_worker,
                             username,
                             password,
                             image_path,
                             decode_mode,
                             i + 1);
    }

    for (auto& t : workers) t.join();
    return 0;
}
