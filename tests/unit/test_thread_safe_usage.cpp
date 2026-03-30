// test_thread_safe_usage.cpp — unit tests for supported parallel usage pattern
// Pattern: one client instance per thread.

#include <atomic>
#include <chrono>
#include <string>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

#include "deathbycaptcha/deathbycaptcha.hpp"

using namespace dbc;

namespace {
int parse_numeric_suffix(const std::string& s) {
    std::size_t pos = s.size();
    while (pos > 0 && std::isdigit(static_cast<unsigned char>(s[pos - 1]))) {
        --pos;
    }
    if (pos == s.size()) return 0;
    return std::stoi(s.substr(pos));
}

void update_max(std::atomic<int>& max_value, int candidate) {
    int current = max_value.load(std::memory_order_relaxed);
    while (current < candidate &&
           !max_value.compare_exchange_weak(current,
                                            candidate,
                                            std::memory_order_relaxed,
                                            std::memory_order_relaxed)) {
    }
}

class ConcurrentHttpMockClient : public HttpClient {
public:
    using HttpClient::HttpClient;

    static std::atomic<int> in_flight;
    static std::atomic<int> max_in_flight;

protected:
    std::string http_call(
        std::string_view method,
        std::string_view endpoint,
        const Params& fields,
        const std::vector<std::pair<std::string, std::vector<std::uint8_t>>>&)
        override {
        if (method == "POST" && endpoint == "user") {
            const int now = in_flight.fetch_add(1, std::memory_order_relaxed) + 1;
            update_max(max_in_flight, now);

            std::this_thread::sleep_for(std::chrono::milliseconds(30));

            const int idx = parse_numeric_suffix(fields.at("username"));
            const std::string response =
                "{\"user\":" + std::to_string(idx) +
                ",\"balance\":1.0,\"rate\":0.139,\"is_banned\":false}";

            in_flight.fetch_sub(1, std::memory_order_relaxed);
            return response;
        }
        return "{}";
    }
};

std::atomic<int> ConcurrentHttpMockClient::in_flight{0};
std::atomic<int> ConcurrentHttpMockClient::max_in_flight{0};

class ConcurrentSocketMockClient : public SocketClient {
public:
    using SocketClient::SocketClient;

    static std::atomic<int> in_flight;
    static std::atomic<int> max_in_flight;

protected:
    Params socket_call(std::string_view command, Params) override {
        if (command == "login") {
            return Params{};
        }

        if (command == "user") {
            const int now = in_flight.fetch_add(1, std::memory_order_relaxed) + 1;
            update_max(max_in_flight, now);

            std::this_thread::sleep_for(std::chrono::milliseconds(30));

            in_flight.fetch_sub(1, std::memory_order_relaxed);
            return Params{{"user", "1"},
                          {"rate", "0.139"},
                          {"balance", "1.0"},
                          {"is_banned", "false"}};
        }

        return Params{};
    }
};

std::atomic<int> ConcurrentSocketMockClient::in_flight{0};
std::atomic<int> ConcurrentSocketMockClient::max_in_flight{0};
} // namespace

TEST(ThreadSafeUsageTest, HttpOneInstancePerThreadRunsConcurrently) {
    constexpr int kThreads = 8;
    ConcurrentHttpMockClient::in_flight.store(0, std::memory_order_relaxed);
    ConcurrentHttpMockClient::max_in_flight.store(0, std::memory_order_relaxed);

    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::vector<long long> users(kThreads, -1);
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            ConcurrentHttpMockClient client("user" + std::to_string(i + 1), "pass");
            ready.fetch_add(1, std::memory_order_relaxed);
            while (!go.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            users[i] = client.get_user().user;
        });
    }

    while (ready.load(std::memory_order_relaxed) < kThreads) {
        std::this_thread::yield();
    }
    go.store(true, std::memory_order_release);

    for (auto& t : threads) t.join();

    for (int i = 0; i < kThreads; ++i) {
        EXPECT_EQ(users[i], static_cast<long long>(i + 1));
    }
    EXPECT_GE(ConcurrentHttpMockClient::max_in_flight.load(std::memory_order_relaxed), 2);
}

TEST(ThreadSafeUsageTest, SocketOneInstancePerThreadRunsConcurrently) {
    constexpr int kThreads = 8;
    ConcurrentSocketMockClient::in_flight.store(0, std::memory_order_relaxed);
    ConcurrentSocketMockClient::max_in_flight.store(0, std::memory_order_relaxed);

    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::vector<double> balances(kThreads, -1.0);
    std::vector<std::thread> threads;
    threads.reserve(kThreads);

    for (int i = 0; i < kThreads; ++i) {
        threads.emplace_back([&, i] {
            ConcurrentSocketMockClient client("user", "pass");
            ready.fetch_add(1, std::memory_order_relaxed);
            while (!go.load(std::memory_order_acquire)) {
                std::this_thread::yield();
            }
            balances[i] = client.get_user().balance;
        });
    }

    while (ready.load(std::memory_order_relaxed) < kThreads) {
        std::this_thread::yield();
    }
    go.store(true, std::memory_order_release);

    for (auto& t : threads) t.join();

    for (double b : balances) {
        EXPECT_DOUBLE_EQ(b, 1.0);
    }
    EXPECT_GE(ConcurrentSocketMockClient::max_in_flight.load(std::memory_order_relaxed), 2);
}
