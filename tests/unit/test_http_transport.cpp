/// test_http_transport.cpp
/// Tests that exercise the REAL HttpClient::http_call() implementation by
/// spinning up a minimal in-process HTTP/1.1 server on a random localhost port.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>
#include <cstring>
#include <deque>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <fcntl.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "deathbycaptcha/deathbycaptcha.hpp"

using namespace dbc;

// ─── Minimal in-process HTTP/1.1 server ──────────────────────────────────────

struct HttpResp {
    int         status{200};
    std::string body;
};

class MiniHttpServer {
public:
    MiniHttpServer() {
        listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port        = 0; // let OS choose
        ::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        ::listen(listen_fd_, 16);
        socklen_t len = sizeof(addr);
        ::getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
        port_ = ntohs(addr.sin_port);
    }

    ~MiniHttpServer() { stop(); }

    int port() const { return port_; }

    std::string base_url() const {
        return "http://127.0.0.1:" + std::to_string(port_) + "/api";
    }

    void queue(HttpResp r) {
        std::lock_guard<std::mutex> lk(mx_);
        responses_.push_back(std::move(r));
    }

    void start() {
        running_ = true;
        thr_ = std::thread([this] { serve_loop(); });
    }

    void stop() {
        running_ = false;
        // Unblock accept() by connecting and immediately closing
        if (listen_fd_ >= 0) {
            int tmp = ::socket(AF_INET, SOCK_STREAM, 0);
            if (tmp >= 0) {
                sockaddr_in addr{};
                addr.sin_family      = AF_INET;
                addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
                addr.sin_port        = htons(static_cast<uint16_t>(port_));
                ::connect(tmp, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
                ::close(tmp);
            }
        }
        if (thr_.joinable()) thr_.join();
        if (listen_fd_ >= 0) { ::close(listen_fd_); listen_fd_ = -1; }
    }

private:
    void serve_loop() {
        while (running_) {
            sockaddr_in cli{};
            socklen_t   len = sizeof(cli);
            int conn = ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&cli), &len);
            if (conn < 0 || !running_) {
                if (conn >= 0) ::close(conn);
                break;
            }
            handle(conn);
            ::close(conn);
        }
    }

    void handle(int conn) {
        // Read until end-of-headers (\r\n\r\n)
        std::string hdr;
        char ch;
        while (hdr.find("\r\n\r\n") == std::string::npos) {
            int n = ::recv(conn, &ch, 1, 0);
            if (n <= 0) return;
            hdr += ch;
            if (hdr.size() > 65536) return;
        }

        // Drain POST body: parse Content-Length from headers
        {
            auto pos = hdr.find("Content-Length:");
            if (pos == std::string::npos) pos = hdr.find("content-length:");
            if (pos != std::string::npos) {
                pos += 15; // skip "Content-Length:"
                while (pos < hdr.size() && hdr[pos] == ' ') ++pos;
                long cl = 0;
                try { cl = std::stol(hdr.substr(pos)); } catch (...) {}
                if (cl > 0) {
                    std::vector<char> body(static_cast<size_t>(cl));
                    size_t got = 0;
                    while (got < static_cast<size_t>(cl)) {
                        int n = ::recv(conn, body.data() + got,
                                       static_cast<int>(cl - static_cast<long>(got)), 0);
                        if (n <= 0) return;
                        got += static_cast<size_t>(n);
                    }
                }
            }
        }

        // Pop next queued response
        HttpResp resp;
        {
            std::lock_guard<std::mutex> lk(mx_);
            if (!responses_.empty()) {
                resp = responses_.front();
                responses_.pop_front();
            } else {
                resp = {200, R"({"status":"ok"})"};
            }
        }

        const char* status_str = "OK";
        if (resp.status == 400) status_str = "Bad Request";
        else if (resp.status == 403) status_str = "Forbidden";
        else if (resp.status == 413) status_str = "Request Entity Too Large";
        else if (resp.status == 503) status_str = "Service Unavailable";
        else if (resp.status >= 500) status_str = "Internal Server Error";

        std::string http_resp =
            "HTTP/1.1 " + std::to_string(resp.status) + " " + status_str + "\r\n"
            "Content-Type: application/json\r\n"
            "Content-Length: " + std::to_string(resp.body.size()) + "\r\n"
            "Connection: close\r\n"
            "\r\n" +
            resp.body;

        std::size_t sent = 0;
        while (sent < http_resp.size()) {
            int n = ::send(conn, http_resp.c_str() + sent,
                           static_cast<int>(http_resp.size() - sent),
                           MSG_NOSIGNAL);
            if (n <= 0) break;
            sent += static_cast<size_t>(n);
        }
    }

    int             listen_fd_{-1};
    int             port_{0};
    std::atomic<bool> running_{false};
    std::thread     thr_;
    std::mutex      mx_;
    std::deque<HttpResp> responses_;
};

// ─── Subclass that points to the local test server ────────────────────────────

class LocalHttpClient : public HttpClient {
public:
    LocalHttpClient(const std::string& user, const std::string& pass,
                    const std::string& base)
        : HttpClient(user, pass) { m_http_base_url = base; }

    explicit LocalHttpClient(const std::string& token, const std::string& base)
        : HttpClient(token) { m_http_base_url = base; }
};

// ─── Test fixture ─────────────────────────────────────────────────────────────

class HttpTransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        srv_.start();
        client_ = std::make_unique<LocalHttpClient>("user", "pass", srv_.base_url());
        client_->is_verbose = false;
    }
    void TearDown() override { srv_.stop(); }

    MiniHttpServer                  srv_;
    std::unique_ptr<LocalHttpClient> client_;
};

// ─── Tests ────────────────────────────────────────────────────────────────────

TEST_F(HttpTransportTest, GetUserPostWithFields) {
    srv_.queue({200, R"({"user":42,"rate":0.139,"balance":9.5,"is_banned":false})"});
    User u = client_->get_user();
    EXPECT_EQ(u.user, 42LL);
    EXPECT_NEAR(u.balance, 9.5, 0.001);
    EXPECT_FALSE(u.is_banned);
}

TEST_F(HttpTransportTest, GetUserAuthtokenPath) {
    LocalHttpClient tc("my_token", srv_.base_url());
    tc.is_verbose = false;
    srv_.queue({200, R"({"user":7,"rate":0.5,"balance":1.0,"is_banned":false})"});
    User u = tc.get_user();
    EXPECT_EQ(u.user, 7LL);
}

TEST_F(HttpTransportTest, GetCaptchaGet) {
    srv_.queue({200, R"({"captcha":99,"text":"hello","is_correct":true})"});
    Captcha c = client_->get_captcha(99);
    EXPECT_EQ(c.captcha, 99LL);
    ASSERT_TRUE(c.text.has_value());
    EXPECT_EQ(*c.text, "hello");
    EXPECT_TRUE(c.is_correct);
}

TEST_F(HttpTransportTest, ReportPost) {
    srv_.queue({200, R"({"captcha":5,"is_correct":false})"});
    EXPECT_TRUE(client_->report(5));
}

TEST_F(HttpTransportTest, UploadSpanMultipart) {
    srv_.queue({200, R"({"captcha":77,"text":null,"is_correct":false})"});
    std::vector<std::uint8_t> data = {0x89, 0x50, 0x4E, 0x47}; // PNG magic
    Captcha c = client_->upload(std::span<const std::uint8_t>(data));
    EXPECT_EQ(c.captcha, 77LL);
    EXPECT_FALSE(c.text.has_value());
}

TEST_F(HttpTransportTest, UploadFileMultipart) {
    char tmp[] = "/tmp/dbc_http_test_XXXXXX";
    int fd = mkstemp(tmp);
    const char content[] = "fake_image_data_for_test";
    ::write(fd, content, static_cast<size_t>(strlen(content)));
    ::close(fd);

    srv_.queue({200, R"({"captcha":88,"text":null,"is_correct":false})"});
    Captcha c = client_->upload(std::filesystem::path(tmp));
    EXPECT_EQ(c.captcha, 88LL);
    ::unlink(tmp);
}

TEST_F(HttpTransportTest, UploadTokenTypeUrlEncoded) {
    // Empty span → url-encoded POST (no multipart)
    srv_.queue({200, R"({"captcha":100,"text":null,"is_correct":false})"});
    Params p = {{"type", "4"}, {"token_params", "{}"}};
    Captcha c = client_->upload(std::span<const std::uint8_t>{}, p);
    EXPECT_EQ(c.captcha, 100LL);
}

TEST_F(HttpTransportTest, VerboseLoggingCoversLogCalls) {
    client_->is_verbose = true;
    srv_.queue({200, R"({"user":1,"rate":0.1,"balance":0.5,"is_banned":true})"});
    User u = client_->get_user();
    EXPECT_TRUE(u.is_banned);
}

// ── HTTP error codes ──────────────────────────────────────────────────────────

TEST_F(HttpTransportTest, Http403ThrowsAccessDenied) {
    srv_.queue({403, ""});
    EXPECT_THROW(client_->get_user(), AccessDeniedException);
}

TEST_F(HttpTransportTest, Http400ThrowsInvalidArgument) {
    srv_.queue({400, ""});
    EXPECT_THROW(client_->get_user(), std::invalid_argument);
}

TEST_F(HttpTransportTest, Http413ThrowsInvalidArgument) {
    srv_.queue({413, ""});
    EXPECT_THROW(client_->get_user(), std::invalid_argument);
}

TEST_F(HttpTransportTest, Http503ThrowsServiceOverload) {
    srv_.queue({503, ""});
    EXPECT_THROW(client_->get_user(), ServiceOverloadException);
}

TEST_F(HttpTransportTest, Http500ThrowsRuntimeError) {
    srv_.queue({500, ""});
    EXPECT_THROW(client_->get_user(), std::runtime_error);
}

// ── parse_response error paths ────────────────────────────────────────────────

TEST_F(HttpTransportTest, EmptyBodyThrowsRuntimeError) {
    srv_.queue({200, ""});
    EXPECT_THROW(client_->get_user(), std::runtime_error);
}

TEST_F(HttpTransportTest, NonJsonBodyThrowsRuntimeError) {
    srv_.queue({200, "not_valid_json"});
    EXPECT_THROW(client_->get_user(), std::runtime_error);
}

// ── Curl-level failure (nothing listening on the target port) ─────────────────

TEST(HttpCurlErrorTest, ThrowsRuntimeOnCurlFail) {
    // Port 1 is reserved/not-listening on any non-root system → ECONNREFUSED
    LocalHttpClient c("u", "p", "http://127.0.0.1:1/api");
    c.is_verbose = false;
    EXPECT_THROW(c.get_user(), std::runtime_error);
}
