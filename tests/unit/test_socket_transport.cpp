/// test_socket_transport.cpp
/// Tests that exercise the REAL SocketClient transport layer (SocketClientImpl::
/// connect_to_host, sendrecv, socket_call error responses, retry logic) by
/// spinning up a minimal in-process TCP server that speaks the DBC CRLF-JSON
/// protocol.

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <atomic>
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

// ─── Minimal in-process TCP server (DBC CRLF-JSON protocol) ──────────────────
//
// • Each accepted connection is kept alive until the client closes it.
// • For every line (terminated by \r\n) received, pops one entry from the
//   response queue and sends it back as "<json>\r\n".
// • Special sentinel "__DISCONNECT__": closes the connection without replying
//   (used to exercise the retry-on-disconnect path).

class MiniSocketServer {
public:
    MiniSocketServer() {
        listen_fd_ = ::socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        ::setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        sockaddr_in addr{};
        addr.sin_family      = AF_INET;
        addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
        addr.sin_port        = 0;
        ::bind(listen_fd_, reinterpret_cast<sockaddr*>(&addr), sizeof(addr));
        ::listen(listen_fd_, 16);
        socklen_t len = sizeof(addr);
        ::getsockname(listen_fd_, reinterpret_cast<sockaddr*>(&addr), &len);
        port_ = ntohs(addr.sin_port);
    }

    ~MiniSocketServer() { stop(); }

    int  port() const { return port_; }

    void queue(std::string json_response) {
        std::lock_guard<std::mutex> lk(mx_);
        responses_.push_back(std::move(json_response));
    }

    void start() {
        running_ = true;
        thr_ = std::thread([this] { serve_loop(); });
    }

    void stop() {
        running_ = false;
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
            socklen_t len = sizeof(cli);
            int conn = ::accept(listen_fd_, reinterpret_cast<sockaddr*>(&cli), &len);
            if (conn < 0 || !running_) {
                if (conn >= 0) ::close(conn);
                break;
            }
            handle_conn(conn);
            ::close(conn);
        }
    }

    void handle_conn(int conn) {
        std::string buf;
        char ch;
        while (true) {
            int n = ::recv(conn, &ch, 1, 0);
            if (n <= 0) return; // client closed
            buf += ch;
            // Complete line received?
            if (buf.size() >= 2 &&
                buf[buf.size() - 2] == '\r' &&
                buf[buf.size() - 1] == '\n') {
                buf.resize(buf.size() - 2); // strip CRLF

                std::string resp;
                {
                    std::lock_guard<std::mutex> lk(mx_);
                    if (!responses_.empty()) {
                        resp = responses_.front();
                        responses_.pop_front();
                    } else {
                        resp = "{}";
                    }
                }

                if (resp == "__DISCONNECT__") return; // close connection

                const std::string wire = resp + "\r\n";
                ::send(conn, wire.c_str(), static_cast<int>(wire.size()), MSG_NOSIGNAL);
                buf.clear();
            }
            if (buf.size() > 65536) return; // safety
        }
    }

    int              listen_fd_{-1};
    int              port_{0};
    std::atomic<bool> running_{false};
    std::thread      thr_;
    std::mutex       mx_;
    std::deque<std::string> responses_;
};

// ─── Subclass that can redirect the socket endpoint ───────────────────────────

class RealSocketClient : public SocketClient {
public:
    using SocketClient::SocketClient;

    void configure(const std::string& host, int port) {
        set_test_socket_endpoint(host, port);
    }

    // Expose protected socket_call for direct error-response tests
    Params call(std::string_view cmd, Params data = {}) {
        return socket_call(cmd, std::move(data));
    }
};

// ─── Helpers ──────────────────────────────────────────────────────────────────

static std::string user_json(long long id = 1, double bal = 10.0) {
    return R"({"user":")" + std::to_string(id) +
           R"(","rate":"0.139","balance":")" + std::to_string(bal) +
           R"(","is_banned":"false"})";
}

static std::string captcha_json(long long id = 55,
                                const std::string& text = "") {
    if (text.empty())
        return R"({"captcha":")" + std::to_string(id) + R"(","is_correct":"false"})";
    return R"({"captcha":")" + std::to_string(id) +
           R"(","text":")" + text + R"(","is_correct":"true"})";
}

// ─── Test fixture ─────────────────────────────────────────────────────────────

class SocketTransportTest : public ::testing::Test {
protected:
    void SetUp() override {
        srv_.start();
        client_ = std::make_unique<RealSocketClient>("user", "pass");
        client_->configure("127.0.0.1", srv_.port());
        client_->is_verbose = false;
    }
    void TearDown() override {
        client_.reset();
        srv_.stop();
    }

    MiniSocketServer                  srv_;
    std::unique_ptr<RealSocketClient> client_;
};

// ─── Tests ────────────────────────────────────────────────────────────────────

// ── Happy-path commands ───────────────────────────────────────────────────────

TEST_F(SocketTransportTest, GetUserRealTransport) {
    srv_.queue("{}");                  // login
    srv_.queue(user_json(42, 9.5));    // user
    User u = client_->get_user();
    EXPECT_EQ(u.user, 42LL);
    EXPECT_NEAR(u.balance, 9.5, 0.001);
}

TEST_F(SocketTransportTest, GetUserVerbose) {
    client_->is_verbose = true;
    srv_.queue("{}");
    srv_.queue(user_json(1, 5.0));
    User u = client_->get_user();
    EXPECT_EQ(u.user, 1LL);
}

TEST_F(SocketTransportTest, GetCaptchaSolvedText) {
    srv_.queue("{}");                          // login
    srv_.queue(captcha_json(99, "token_abc")); // captcha
    Captcha c = client_->get_captcha(99);
    EXPECT_EQ(c.captcha, 99LL);
    ASSERT_TRUE(c.text.has_value());
    EXPECT_EQ(*c.text, "token_abc");
}

TEST_F(SocketTransportTest, GetCaptchaPending) {
    srv_.queue("{}");
    srv_.queue(captcha_json(10));
    Captcha c = client_->get_captcha(10);
    EXPECT_EQ(c.captcha, 10LL);
    EXPECT_FALSE(c.text.has_value());
}

TEST_F(SocketTransportTest, ReportCommand) {
    srv_.queue("{}");                                           // login
    srv_.queue(R"({"captcha":"5","is_correct":"false"})");     // report
    EXPECT_TRUE(client_->report(5));
}

TEST_F(SocketTransportTest, UploadSpanUsesBase64) {
    srv_.queue("{}"); // login
    srv_.queue(R"({"captcha":"200","is_correct":"false"})"); // upload
    std::vector<std::uint8_t> data = {0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A};
    Captcha c = client_->upload(std::span<const std::uint8_t>(data));
    EXPECT_EQ(c.captcha, 200LL);
}

TEST_F(SocketTransportTest, UploadFilePathUsesReadFileBytesAndBase64) {
    char tmp[] = "/tmp/dbc_sock_test_XXXXXX";
    int fd = mkstemp(tmp);
    const char content[] = "some_binary_data";
    ::write(fd, content, static_cast<size_t>(strlen(content)));
    ::close(fd);

    srv_.queue("{}");  // login
    srv_.queue(R"({"captcha":"201","is_correct":"false"})");
    Captcha c = client_->upload(std::filesystem::path(tmp));
    EXPECT_EQ(c.captcha, 201LL);
    ::unlink(tmp);
}

TEST_F(SocketTransportTest, EnsureLoggedInCalledOnceOnly) {
    // Two calls to get_* should only trigger one login
    srv_.queue("{}");               // login (once)
    srv_.queue(user_json(1, 5.0));  // user
    srv_.queue(captcha_json(7));    // captcha (no extra login)
    client_->get_user();
    client_->get_captcha(7);
}

TEST_F(SocketTransportTest, CloseResetsConnection) {
    srv_.queue("{}"); // login
    srv_.queue(user_json());
    client_->get_user();
    client_->close(); // disconnect; logged_in = false

    // After close, next call re-logs in
    srv_.queue("{}"); // login again
    srv_.queue(user_json(3, 2.0));
    User u = client_->get_user();
    EXPECT_EQ(u.user, 3LL);
}

// ── Retry on disconnect ───────────────────────────────────────────────────────

TEST_F(SocketTransportTest, RetriesOnceAfterRecvReturnsZero) {
    // FIFO: login OK, user→disconnect (server closes conn), user OK (new conn)
    srv_.queue("{}");               // login on conn-1
    srv_.queue("__DISCONNECT__");   // server closes conn-1 mid-user
    srv_.queue(user_json(77, 3.0)); // user on conn-2 (retry)
    User u = client_->get_user();
    EXPECT_EQ(u.user, 77LL);
}

// ── Error responses from API ──────────────────────────────────────────────────

TEST_F(SocketTransportTest, ErrorNotLoggedInThrowsAccessDenied) {
    srv_.queue(R"({"error":"not-logged-in"})");
    EXPECT_THROW(client_->call("login"), AccessDeniedException);
}

TEST_F(SocketTransportTest, ErrorBannedThrowsAccessDenied) {
    srv_.queue(R"({"error":"banned"})");
    EXPECT_THROW(client_->call("login"), AccessDeniedException);
}

TEST_F(SocketTransportTest, ErrorInsufficientFundsThrowsAccessDenied) {
    srv_.queue(R"({"error":"insufficient-funds"})");
    EXPECT_THROW(client_->call("upload"), AccessDeniedException);
}

TEST_F(SocketTransportTest, ErrorServiceOverloadThrowsServiceOverload) {
    srv_.queue(R"({"error":"service-overload"})");
    EXPECT_THROW(client_->call("login"), ServiceOverloadException);
}

TEST_F(SocketTransportTest, ErrorUnknownThrowsRuntimeError) {
    srv_.queue(R"({"error":"some-unknown-error"})");
    EXPECT_THROW(client_->call("login"), std::runtime_error);
}

TEST_F(SocketTransportTest, ErrorInvalidCredentialsThrowsAccessDenied) {
    srv_.queue(R"({"error":"invalid-credentials"})");
    EXPECT_THROW(client_->call("login"), AccessDeniedException);
}

// ── Connection failure ────────────────────────────────────────────────────────

TEST(SocketConnectFailTest, ThrowsRuntimeErrorWhenConnectRefused) {
    // Grab a port that used to be listening but is now closed
    int bound_port = 0;
    {
        MiniSocketServer tmp_srv; // binds to a port
        bound_port = tmp_srv.port();
    } // destructor closes the listening socket → port is free/refused

    RealSocketClient c("u", "p");
    c.configure("127.0.0.1", bound_port);
    c.is_verbose = false;
    // Both retry attempts will fail → std::runtime_error
    EXPECT_THROW(c.call("login"), std::runtime_error);
}
