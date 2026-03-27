/// test_api_basic.cpp — live integration tests (require DBC_USERNAME / DBC_PASSWORD)
///
/// These tests call the real DeathByCaptcha API.  They are skipped
/// automatically when the environment variables DBC_USERNAME and
/// DBC_PASSWORD are not set, following the same skip pattern as all
/// other official DBC client repos.

#include <gtest/gtest.h>
#include <cstdlib>
#include <filesystem>
#include "deathbycaptcha/deathbycaptcha.hpp"

// ── Helpers ───────────────────────────────────────────────────────────────────
static std::string env_or_empty(const char* name) {
    const char* val = std::getenv(name);
    return val ? val : "";
}

static bool credentials_available() {
    return !env_or_empty("DBC_USERNAME").empty() &&
           !env_or_empty("DBC_PASSWORD").empty();
}

// ── Fixture ───────────────────────────────────────────────────────────────────
class ApiIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!credentials_available()) {
            GTEST_SKIP() << "DBC_USERNAME / DBC_PASSWORD not set. "
                            "Skipping live integration test.";
        }
        m_username = env_or_empty("DBC_USERNAME");
        m_password = env_or_empty("DBC_PASSWORD");
    }

    std::string m_username;
    std::string m_password;
};

// ── Tests: HttpClient ─────────────────────────────────────────────────────────
TEST_F(ApiIntegrationTest, HttpGetBalancePositive) {
    dbc::HttpClient client(m_username, m_password);
    client.is_verbose = false;
    double bal = client.get_balance();
    EXPECT_GT(bal, 0.0) << "Balance should be > 0 for a funded account";
}

TEST_F(ApiIntegrationTest, HttpGetUserReturnsNonZeroId) {
    dbc::HttpClient client(m_username, m_password);
    client.is_verbose = false;
    dbc::User u = client.get_user();
    EXPECT_NE(u.user, 0LL) << "User ID should be non-zero";
    EXPECT_FALSE(u.is_banned);
}

TEST_F(ApiIntegrationTest, HttpUploadAndDecodeNormalCaptcha) {
    const std::filesystem::path fixture = "tests/fixtures/test.jpg";
    if (!std::filesystem::exists(fixture)) {
        GTEST_SKIP() << "Fixture tests/fixtures/test.jpg not found";
    }
    dbc::HttpClient client(m_username, m_password);
    client.is_verbose = false;
    auto result = client.decode(fixture, 120);
    ASSERT_TRUE(result.has_value())
        << "decode() should return a solved captcha";
    EXPECT_NE(result->captcha, 0LL);
    EXPECT_TRUE(result->text.has_value());
    EXPECT_FALSE(result->text->empty());
}

// ── Tests: SocketClient ───────────────────────────────────────────────────────
TEST_F(ApiIntegrationTest, SocketGetBalancePositive) {
    dbc::SocketClient client(m_username, m_password);
    client.is_verbose = false;
    double bal = client.get_balance();
    EXPECT_GT(bal, 0.0);
}

TEST_F(ApiIntegrationTest, SocketGetUserReturnsNonZeroId) {
    dbc::SocketClient client(m_username, m_password);
    client.is_verbose = false;
    dbc::User u = client.get_user();
    EXPECT_NE(u.user, 0LL);
}
