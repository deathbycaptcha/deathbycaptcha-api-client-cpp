/// test_socket_client.cpp — SocketClient unit tests with mock socket_call

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "deathbycaptcha/deathbycaptcha.hpp"

using namespace dbc;

// ── TestableSocketClient — replaces socket_call with a mock ──────────────────
class TestableSocketClient : public SocketClient {
public:
    using SocketClient::SocketClient;

    MOCK_METHOD(Params, socket_call,
        (std::string_view command, Params data), (override));
};

// ── Helpers ───────────────────────────────────────────────────────────────────
static Params make_user_params(long long id = 1, double balance = 10.0) {
    return {
        {"user",      std::to_string(id)},
        {"rate",      "0.139"},
        {"balance",   std::to_string(balance)},
        {"is_banned", "false"}
    };
}

static Params make_captcha_params(long long id = 55,
                                  const std::string& text = "") {
    Params p = {
        {"captcha",    std::to_string(id)},
        {"is_correct", text.empty() ? "false" : "true"}
    };
    if (!text.empty()) p["text"] = text;
    return p;
}

// ── Construction ──────────────────────────────────────────────────────────────
TEST(SocketClientTest, ConstructUsernamePassword) {
    TestableSocketClient c("user", "pass");
    (void)c;
}

TEST(SocketClientTest, ConstructAuthtoken) {
    TestableSocketClient c("my_token");
    (void)c;
}

// ── close is idempotent ───────────────────────────────────────────────────────
TEST(SocketClientTest, CloseIsIdempotent) {
    TestableSocketClient c("u", "p");
    c.close();
    c.close();
}

// ── get_user ──────────────────────────────────────────────────────────────────
TEST(SocketClientTest, GetUserCallsLoginThenUser) {
    TestableSocketClient c("user", "pass");
    using ::testing::_;
    using ::testing::Return;
    // First call: login
    EXPECT_CALL(c, socket_call("login", _))
        .WillOnce(Return(Params{}));
    // Second call: user
    EXPECT_CALL(c, socket_call("user", _))
        .WillOnce(Return(make_user_params(42, 9.5)));

    User u = c.get_user();
    EXPECT_EQ(u.user, 42LL);
    EXPECT_DOUBLE_EQ(u.balance, 9.5);
}

// ── get_captcha ───────────────────────────────────────────────────────────────
TEST(SocketClientTest, GetCaptchaReturnsNullTextWhenPending) {
    TestableSocketClient c("user", "pass");
    using ::testing::_;
    using ::testing::Return;
    EXPECT_CALL(c, socket_call("login", _)).WillOnce(Return(Params{}));
    EXPECT_CALL(c, socket_call("captcha", _))
        .WillOnce(Return(make_captcha_params(10, "")));

    Captcha cap = c.get_captcha(10);
    EXPECT_EQ(cap.captcha, 10LL);
    EXPECT_FALSE(cap.text.has_value());
}

TEST(SocketClientTest, GetCaptchaReturnsSolvedText) {
    TestableSocketClient c("user", "pass");
    using ::testing::_;
    using ::testing::Return;
    EXPECT_CALL(c, socket_call("login", _)).WillOnce(Return(Params{}));
    EXPECT_CALL(c, socket_call("captcha", _))
        .WillOnce(Return(make_captcha_params(99, "token_abc")));

    Captcha cap = c.get_captcha(99);
    ASSERT_TRUE(cap.text.has_value());
    EXPECT_EQ(*cap.text, "token_abc");
    EXPECT_TRUE(cap.is_correct);
}

// ── report ────────────────────────────────────────────────────────────────────
TEST(SocketClientTest, ReportCallsReportCommand) {
    TestableSocketClient c("user", "pass");
    using ::testing::_;
    using ::testing::Return;
    EXPECT_CALL(c, socket_call("login", _)).WillOnce(Return(Params{}));
    EXPECT_CALL(c, socket_call("report", _))
        .WillOnce(Return(Params{{"is_correct","false"}}));

    EXPECT_TRUE(c.report(5));
}

// ── AccessDeniedException propagates ──────────────────────────────────────────
TEST(SocketClientTest, ThrowsAccessDeniedFromSocketCall) {
    TestableSocketClient c("user", "pass");
    using ::testing::_;
    using ::testing::Throw;
    EXPECT_CALL(c, socket_call("login", _))
        .WillOnce(Throw(AccessDeniedException("bad")));

    EXPECT_THROW(c.get_user(), AccessDeniedException);
}

// ── ServiceOverloadException propagates ──────────────────────────────────────
TEST(SocketClientTest, ThrowsServiceOverloadFromSocketCall) {
    TestableSocketClient c("user", "pass");
    using ::testing::_;
    using ::testing::Throw;
    EXPECT_CALL(c, socket_call("login", _))
        .WillOnce(Throw(ServiceOverloadException("overload")));

    EXPECT_THROW(c.get_user(), ServiceOverloadException);
}
