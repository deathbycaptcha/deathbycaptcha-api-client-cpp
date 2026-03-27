/// test_client_core.cpp — base Client behaviour tests (mocked sub-classes)

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "deathbycaptcha/deathbycaptcha.hpp"

using namespace dbc;
using ::testing::Return;

// ── Minimal concrete mock of Client ─────────────────────────────────────────
class MockClient : public Client {
public:
    MockClient(std::string u, std::string p) : Client(std::move(u), std::move(p)) {}
    explicit MockClient(std::string tok) : Client(std::move(tok)) {}

    MOCK_METHOD(User,    get_user,   (), (override));
    MOCK_METHOD(Captcha, get_captcha,(long long), (override));
    MOCK_METHOD(bool,    report,     (long long), (override));
    MOCK_METHOD(Captcha, upload,
        (const std::filesystem::path&, Params), (override));
    MOCK_METHOD(Captcha, upload,
        (std::span<const std::uint8_t>, Params), (override));
    MOCK_METHOD(std::optional<Captcha>, decode,
        (std::optional<std::filesystem::path>, int, Params), (override));
};

// ── Construction tests ────────────────────────────────────────────────────────
TEST(ClientCore, ConstructWithUsernamePassword) {
    MockClient c("my_user", "my_pass");
    // No exception — access to auth params
    (void)c;
}

TEST(ClientCore, ConstructWithAuthtoken) {
    MockClient c("my_token");
    (void)c;
}

// ── get_balance delegates to get_user ─────────────────────────────────────────
TEST(ClientCore, GetBalanceDelegatesToGetUser) {
    MockClient c("user", "pass");
    User u;
    u.balance = 123.45;
    EXPECT_CALL(c, get_user()).WillOnce(Return(u));
    EXPECT_DOUBLE_EQ(c.get_balance(), 123.45);
}

// ── get_text delegates to get_captcha ─────────────────────────────────────────
TEST(ClientCore, GetTextReturnsSolvedText) {
    MockClient c("user", "pass");
    Captcha cap;
    cap.captcha = 11;
    cap.text    = "abc123";
    EXPECT_CALL(c, get_captcha(11)).WillOnce(Return(cap));
    auto t = c.get_text(11);
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(*t, "abc123");
}

TEST(ClientCore, GetTextReturnsNulloptWhenPending) {
    MockClient c("user", "pass");
    Captcha cap;
    cap.captcha = 22;
    EXPECT_CALL(c, get_captcha(22)).WillOnce(Return(cap));
    EXPECT_FALSE(c.get_text(22).has_value());
}

// ── is_verbose flag ───────────────────────────────────────────────────────────
TEST(ClientCore, IsVerboseDefaultTrue) {
    MockClient c("user", "pass");
    EXPECT_TRUE(c.is_verbose);
}

TEST(ClientCore, IsVerboseCanBeDisabled) {
    MockClient c("user", "pass");
    c.is_verbose = false;
    EXPECT_FALSE(c.is_verbose);
}

// ── close() is safe to call multiple times ────────────────────────────────────
TEST(ClientCore, CloseIsIdempotent) {
    MockClient c("user", "pass");
    c.close();
    c.close();
}
