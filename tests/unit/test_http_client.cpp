/// test_http_client.cpp — HttpClient unit tests with mock http_call

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include "deathbycaptcha/deathbycaptcha.hpp"

using namespace dbc;

// ── TestableHttpClient subclass — injects mock HTTP transport ─────────────────
class TestableHttpClient : public HttpClient {
public:
    using HttpClient::HttpClient;

    void set_response(const std::string& body) { m_mock_response = body; }
    void set_throw(std::exception_ptr ex)       { m_throw = ex; }

    // Track last call info
    std::string last_method;
    std::string last_endpoint;
    Params      last_fields;

protected:
    std::string http_call(
        std::string_view method,
        std::string_view endpoint,
        const Params& fields,
        const std::vector<std::pair<std::string,
                                    std::vector<std::uint8_t>>>&) override
    {
        if (m_throw) std::rethrow_exception(m_throw);
        last_method   = method;
        last_endpoint = endpoint;
        last_fields   = fields;
        return m_mock_response;
    }

private:
    std::string      m_mock_response;
    std::exception_ptr m_throw;
};

// ── Helpers ───────────────────────────────────────────────────────────────────
static std::string make_user_json(long long id = 42, double balance = 9.5) {
    return R"({"user":)" + std::to_string(id)
         + R"(,"rate":0.139,"balance":)" + std::to_string(balance)
         + R"(,"is_banned":false})";
}

static std::string make_captcha_json(long long id = 77,
                                     const std::string& text = "",
                                     bool is_correct = false) {
    std::string t = text.empty() ? "null" : '"' + text + '"';
    return R"({"captcha":)" + std::to_string(id)
         + R"(,"text":)" + t
         + R"(,"is_correct":)" + (is_correct ? "true" : "false")
         + '}';
}

// ── Construction ──────────────────────────────────────────────────────────────
TEST(HttpClientTest, ConstructUsernamePassword) {
    TestableHttpClient c("user", "pass");
    (void)c;
}

TEST(HttpClientTest, ConstructAuthtoken) {
    TestableHttpClient c("my_token");
    (void)c;
}

// ── get_user ──────────────────────────────────────────────────────────────────
TEST(HttpClientTest, GetUserReturnsCorrectFields) {
    TestableHttpClient c("user", "pass");
    c.set_response(make_user_json(42, 9.5));
    User u = c.get_user();
    EXPECT_EQ(u.user, 42LL);
    EXPECT_DOUBLE_EQ(u.balance, 9.5);
    EXPECT_FALSE(u.is_banned);
}

TEST(HttpClientTest, GetUserPostsToUserEndpoint) {
    TestableHttpClient c("user", "pass");
    c.set_response(make_user_json());
    c.get_user();
    EXPECT_EQ(c.last_method,   "POST");
    EXPECT_EQ(c.last_endpoint, "user");
}

TEST(HttpClientTest, GetUserSendsCredentials) {
    TestableHttpClient c("my_user", "my_pass");
    c.set_response(make_user_json());
    c.get_user();
    EXPECT_EQ(c.last_fields.at("username"), "my_user");
    EXPECT_EQ(c.last_fields.at("password"), "my_pass");
}

TEST(HttpClientTest, GetUserSendsAuthtoken) {
    TestableHttpClient c("my_token");
    c.set_response(make_user_json());
    c.get_user();
    EXPECT_EQ(c.last_fields.at("authtoken"), "my_token");
}

// ── get_balance ───────────────────────────────────────────────────────────────
TEST(HttpClientTest, GetBalanceReturnsUserBalance) {
    TestableHttpClient c("user", "pass");
    c.set_response(make_user_json(1, 77.5));
    EXPECT_DOUBLE_EQ(c.get_balance(), 77.5);
}

// ── get_captcha ───────────────────────────────────────────────────────────────
TEST(HttpClientTest, GetCaptchaReturnsNullTextWhenPending) {
    TestableHttpClient c("user", "pass");
    c.set_response(make_captcha_json(7, "", false));
    Captcha cap = c.get_captcha(7);
    EXPECT_EQ(cap.captcha, 7LL);
    EXPECT_FALSE(cap.text.has_value());
    EXPECT_FALSE(cap.is_correct);
}

TEST(HttpClientTest, GetCaptchaCallsCorrectEndpoint) {
    TestableHttpClient c("user", "pass");
    c.set_response(make_captcha_json(99));
    c.get_captcha(99);
    EXPECT_EQ(c.last_method,   "GET");
    EXPECT_EQ(c.last_endpoint, "captcha/99");
}

TEST(HttpClientTest, GetCaptchaReturnsSolvedText) {
    TestableHttpClient c("user", "pass");
    c.set_response(make_captcha_json(55, "solved", true));
    Captcha cap = c.get_captcha(55);
    ASSERT_TRUE(cap.text.has_value());
    EXPECT_EQ(*cap.text, "solved");
    EXPECT_TRUE(cap.is_correct);
}

// ── get_text ──────────────────────────────────────────────────────────────────
TEST(HttpClientTest, GetTextReturnsNulloptWhenPending) {
    TestableHttpClient c("user", "pass");
    c.set_response(make_captcha_json(3));
    EXPECT_FALSE(c.get_text(3).has_value());
}

// ── report ────────────────────────────────────────────────────────────────────
TEST(HttpClientTest, ReportCallsReportEndpoint) {
    TestableHttpClient c("user", "pass");
    c.set_response(R"({"captcha":5,"is_correct":false})");
    bool ok = c.report(5);
    EXPECT_TRUE(ok);
    EXPECT_EQ(c.last_method,   "POST");
    EXPECT_EQ(c.last_endpoint, "captcha/5/report");
}

// ── AccessDeniedException on 403 mock ────────────────────────────────────────
TEST(HttpClientTest, ThrowsAccessDeniedWhenInjected) {
    TestableHttpClient c("user", "pass");
    c.set_throw(std::make_exception_ptr(
        AccessDeniedException("test 403")));
    EXPECT_THROW(c.get_user(), AccessDeniedException);
}

// ── ServiceOverloadException mock ────────────────────────────────────────────
TEST(HttpClientTest, ThrowsServiceOverloadWhenInjected) {
    TestableHttpClient c("user", "pass");
    c.set_throw(std::make_exception_ptr(
        ServiceOverloadException("503")));
    EXPECT_THROW(c.get_user(), ServiceOverloadException);
}

// ── upload (bytes, token type=4) ──────────────────────────────────────────────
TEST(HttpClientTest, UploadTokenType4SendsCorrectFields) {
    TestableHttpClient c("user", "pass");
    c.set_response(R"({"captcha":200})");
    c.is_verbose = false;
    Params params;
    params["type"]         = "4";
    params["token_params"] = R"({"googlekey":"abc","pageurl":"https://ex.com"})";
    Captcha cap = c.upload(std::span<const std::uint8_t>{}, params);
    EXPECT_EQ(cap.captcha, 200LL);
    EXPECT_EQ(c.last_fields.at("type"), "4");
    EXPECT_FALSE(c.last_fields.at("token_params").empty());
}

// ── decode returns nullopt on timeout (always-pending mock) ───────────────────
TEST(HttpClientTest, DecodeReturnsNulloptOnTimeout) {
    TestableHttpClient c("user", "pass");
    c.is_verbose = false;
    // First call for upload returns captcha id
    // Subsequent polls return pending
    int call_count = 0;
    // We can't easily vary mock responses per call here, so we subclass further.
    // Simpler: provide a captcha that is never solved and a 1-second timeout.
    // We override the upload path by providing a mock response for token upload.
    c.set_response(R"({"captcha":999,"text":null,"is_correct":false})");
    // decode with type token, 1-second timeout
    Params p = {{"type","4"},{"token_params","{}"}};
    // use 1 second timeout — with 1s poll interval this should time out quickly
    auto result = c.decode(std::nullopt, 1, p);
    EXPECT_FALSE(result.has_value());
    (void)call_count;
}
