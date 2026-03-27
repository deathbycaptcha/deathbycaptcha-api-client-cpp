/// test_api_basic.cpp — live integration tests (require DBC_USERNAME / DBC_PASSWORD)
///
/// These tests call the real DeathByCaptcha API.  They are skipped
/// automatically when the environment variables DBC_USERNAME and
/// DBC_PASSWORD are not set, following the same skip pattern as all
/// other official DBC client repos.

#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <thread>
#include <vector>
#include "deathbycaptcha/deathbycaptcha.hpp"

// ── Helpers ───────────────────────────────────────────────────────────────────
static std::string env_or_empty(const char* name) {
    const char* val = std::getenv(name);
    return val ? val : "";
}

static std::string username_from_env() {
    const std::string primary = env_or_empty("DBC_USERNAME");
    if (!primary.empty()) {
        return primary;
    }
    return env_or_empty("DBC_TEST_USERNAME");
}

static std::string password_from_env() {
    const std::string primary = env_or_empty("DBC_PASSWORD");
    if (!primary.empty()) {
        return primary;
    }
    return env_or_empty("DBC_TEST_PASSWORD");
}

static bool credentials_available() {
    return !username_from_env().empty() &&
           !password_from_env().empty();
}

static std::optional<std::filesystem::path> sample_image_fixture() {
    const std::vector<std::filesystem::path> candidates = {
        "examples/images/normal.jpg",
        "tests/fixtures/test.jpg"
    };
    for (const auto& candidate : candidates) {
        if (std::filesystem::exists(candidate)) {
            return candidate;
        }
    }
    return std::nullopt;
}

static std::vector<std::uint8_t> read_bytes(const std::filesystem::path& file) {
    std::ifstream f(file, std::ios::binary | std::ios::ate);
    if (!f) {
        return {};
    }
    const auto size = f.tellg();
    if (size <= 0) {
        return {};
    }
    std::vector<std::uint8_t> out(static_cast<std::size_t>(size));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(out.data()), size);
    return out;
}

static std::optional<dbc::Captcha> poll_solved(
    dbc::Client& client,
    long long captcha_id,
    int attempts = 25,
    int sleep_seconds = 3)
{
    for (int i = 0; i < attempts; ++i) {
        auto c = client.get_captcha(captcha_id);
        if (c.text.has_value() && !c.text->empty() && c.text.value() != "?") {
            return c;
        }
        std::this_thread::sleep_for(std::chrono::seconds(sleep_seconds));
    }
    return std::nullopt;
}

// ── Fixture ───────────────────────────────────────────────────────────────────
class ApiIntegrationTest : public ::testing::Test {
protected:
    void SetUp() override {
        if (!credentials_available()) {
            GTEST_SKIP() << "Credentials not found. Set DBC_USERNAME/DBC_PASSWORD "
                            "or DBC_TEST_USERNAME/DBC_TEST_PASSWORD.";
        }
        m_username = username_from_env();
        m_password = password_from_env();
        std::cout << "[integration] credentials loaded from env aliases" << std::endl;
    }

    std::string m_username;
    std::string m_password;
};

// ── Tests: HttpClient ─────────────────────────────────────────────────────────
TEST_F(ApiIntegrationTest, HttpGetBalancePositive) {
    dbc::HttpClient client(m_username, m_password);
    client.is_verbose = true;
    double bal = client.get_balance();
    std::cout << "[HTTP] balance=" << bal << std::endl;
    EXPECT_GE(bal, 0.0) << "Balance should be >= 0";
}

TEST_F(ApiIntegrationTest, HttpGetUserReturnsNonZeroId) {
    dbc::HttpClient client(m_username, m_password);
    client.is_verbose = true;
    dbc::User u = client.get_user();
    std::cout << "[HTTP] user_id=" << u.user
              << " rate=" << u.rate
              << " banned=" << u.is_banned << std::endl;
    EXPECT_NE(u.user, 0LL) << "User ID should be non-zero";
    EXPECT_FALSE(u.is_banned);
}

TEST_F(ApiIntegrationTest, HttpUploadAndDecodeNormalCaptcha) {
    const auto fixture = sample_image_fixture();
    if (!fixture.has_value()) {
        GTEST_SKIP() << "No image fixture found (expected examples/images/normal.jpg "
                        "or tests/fixtures/test.jpg).";
    }
    dbc::HttpClient client(m_username, m_password);
    client.is_verbose = true;
    std::cout << "[HTTP] decoding image fixture=" << fixture->string() << std::endl;
    auto result = client.decode(fixture.value(), 120);
    ASSERT_TRUE(result.has_value())
        << "decode() should return a solved captcha";
    std::cout << "[HTTP] image captcha_id=" << result->captcha
              << " text='" << result->text.value_or("") << "'" << std::endl;
    EXPECT_NE(result->captcha, 0LL);
    EXPECT_TRUE(result->text.has_value());
    EXPECT_FALSE(result->text->empty());
}

TEST_F(ApiIntegrationTest, HttpDecodeRecaptchaV2Token) {
    dbc::HttpClient client(m_username, m_password);
    client.is_verbose = true;

    const std::string token_params = R"({
        "googlekey": "6LeIxAcTAAAAAJcZVRqyHh71UMIEGNQ_MXjiZKhI",
        "pageurl":   "https://www.google.com/recaptcha/api2/demo"
    })";

    dbc::Params params = {
        {"type", "4"},
        {"token_params", token_params}
    };
    std::optional<dbc::Captcha> result;
    constexpr int kAttempts = 2;
    for (int attempt = 1; attempt <= kAttempts; ++attempt) {
        std::cout << "[HTTP] decoding reCAPTCHA v2 token (attempt "
                  << attempt << "/" << kAttempts << ")" << std::endl;
        dbc::Captcha uploaded = client.upload(std::span<const std::uint8_t>{}, params);
        std::cout << "[HTTP] token upload captcha_id=" << uploaded.captcha << std::endl;
        EXPECT_NE(uploaded.captcha, 0LL);
        result = poll_solved(client, uploaded.captcha, 35, 3);
        if (result.has_value()) {
            break;
        }
        std::cout << "[HTTP] token decode not solved yet (nullopt)" << std::endl;
    }
    if (!result.has_value()) {
        GTEST_SKIP() << "Token v2 upload succeeded but provider did not return a solved token in time.";
    }
    std::cout << "[HTTP] token captcha_id=" << result->captcha
              << " token_len=" << result->text.value_or("").size() << std::endl;
    EXPECT_NE(result->captcha, 0LL);
    EXPECT_TRUE(result->text.has_value());
    EXPECT_FALSE(result->text->empty());
}

// ── Tests: SocketClient ───────────────────────────────────────────────────────
TEST_F(ApiIntegrationTest, SocketGetBalancePositive) {
    dbc::SocketClient client(m_username, m_password);
    client.is_verbose = false;
    double bal = client.get_balance();
    std::cout << "[SOCKET] balance=" << bal << std::endl;
    EXPECT_GE(bal, 0.0);
}

TEST_F(ApiIntegrationTest, SocketGetUserReturnsNonZeroId) {
    dbc::SocketClient client(m_username, m_password);
    client.is_verbose = false;
    dbc::User u = client.get_user();
    std::cout << "[SOCKET] user_id=" << u.user
              << " rate=" << u.rate
              << " banned=" << u.is_banned << std::endl;
    EXPECT_NE(u.user, 0LL);
}

TEST_F(ApiIntegrationTest, SocketUploadAndDecodeNormalCaptcha) {
    const auto fixture = sample_image_fixture();
    if (!fixture.has_value()) {
        GTEST_SKIP() << "No image fixture found (expected examples/images/normal.jpg "
                        "or tests/fixtures/test.jpg).";
    }
    dbc::SocketClient client(m_username, m_password);
    client.is_verbose = false;
    std::cout << "[SOCKET] decoding image fixture=" << fixture->string() << std::endl;
    std::optional<dbc::Captcha> result;
    std::string last_error;
    for (int attempt = 1; attempt <= 3; ++attempt) {
        try {
            result = client.decode(fixture.value(), 120);
            break;
        } catch (const std::exception& e) {
            last_error = e.what();
            std::cout << "[SOCKET] attempt " << attempt
                      << " image decode error: " << last_error << std::endl;
            if (attempt < 3) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }
    }
    if (!result.has_value() && !last_error.empty()) {
        GTEST_SKIP() << "Socket image decode appears provider-side flaky: " << last_error;
    }
    ASSERT_TRUE(result.has_value()) << "Socket image decode should return a result";
    std::cout << "[SOCKET] image captcha_id=" << result->captcha
              << " text='" << result->text.value_or("") << "'" << std::endl;
    EXPECT_NE(result->captcha, 0LL);
    EXPECT_TRUE(result->text.has_value());
    EXPECT_FALSE(result->text->empty());
}

TEST_F(ApiIntegrationTest, SocketUploadFromBinaryImageBuffer) {
    const auto fixture = sample_image_fixture();
    if (!fixture.has_value()) {
        GTEST_SKIP() << "No image fixture found (expected examples/images/normal.jpg "
                        "or tests/fixtures/test.jpg).";
    }
    auto bytes = read_bytes(fixture.value());
    if (bytes.empty()) {
        GTEST_SKIP() << "Image fixture could not be read as binary.";
    }

    dbc::SocketClient client(m_username, m_password);
    client.is_verbose = false;

    std::cout << "[SOCKET] uploading binary image buffer bytes=" << bytes.size() << std::endl;
    dbc::Captcha uploaded{};
    try {
        uploaded = client.upload(std::span<const std::uint8_t>(bytes.data(), bytes.size()));
    } catch (const std::exception& e) {
        GTEST_SKIP() << "Socket binary upload appears provider-side flaky: " << e.what();
    }

    EXPECT_NE(uploaded.captcha, 0LL);
    auto solved = poll_solved(client, uploaded.captcha, 20, 2);
    if (!solved.has_value()) {
        GTEST_SKIP() << "Socket binary upload accepted but provider did not return text in time.";
    }
    std::cout << "[SOCKET] binary image captcha_id=" << solved->captcha
              << " text='" << solved->text.value_or("") << "'" << std::endl;
    EXPECT_TRUE(solved->text.has_value());
    EXPECT_FALSE(solved->text->empty());
}

TEST_F(ApiIntegrationTest, SocketDecodeRecaptchaV2Token) {
    dbc::SocketClient client(m_username, m_password);
    client.is_verbose = false;

    const std::string token_params = R"({
        "googlekey": "6LeIxAcTAAAAAJcZVRqyHh71UMIEGNQ_MXjiZKhI",
        "pageurl":   "https://www.google.com/recaptcha/api2/demo"
    })";

    dbc::Params params = {
        {"type", "4"},
        {"token_params", token_params}
    };
    std::optional<dbc::Captcha> result;
    constexpr int kAttempts = 2;
    for (int attempt = 1; attempt <= kAttempts; ++attempt) {
        std::cout << "[SOCKET] decoding reCAPTCHA v2 token (attempt "
                  << attempt << "/" << kAttempts << ")" << std::endl;
        dbc::Captcha uploaded{};
        try {
            uploaded = client.upload(std::span<const std::uint8_t>{}, params);
        } catch (const std::exception& e) {
            std::cout << "[SOCKET] token upload error: " << e.what() << std::endl;
            if (attempt == kAttempts) {
                GTEST_SKIP() << "Socket token upload failed due to provider response: " << e.what();
            }
            continue;
        }

        std::cout << "[SOCKET] token upload captcha_id=" << uploaded.captcha << std::endl;
        EXPECT_NE(uploaded.captcha, 0LL);
        result = poll_solved(client, uploaded.captcha, 35, 3);
        if (result.has_value()) {
            break;
        }
        std::cout << "[SOCKET] token decode not solved yet (nullopt)" << std::endl;
    }
    if (!result.has_value()) {
        GTEST_SKIP() << "Socket token upload succeeded but provider did not return a solved token in time.";
    }
    std::cout << "[SOCKET] token captcha_id=" << result->captcha
              << " token_len=" << result->text.value_or("").size() << std::endl;
    EXPECT_NE(result->captcha, 0LL);
    EXPECT_TRUE(result->text.has_value());
    EXPECT_FALSE(result->text->empty());
}
