/// test_coverage_boost.cpp — additional edge-case tests to reach >= 80% coverage

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <filesystem>
#include <fstream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "deathbycaptcha/deathbycaptcha.hpp"

using namespace dbc;

// ── Constants correctness ─────────────────────────────────────────────────────
TEST(Constants, ApiVersionNotEmpty) {
    EXPECT_FALSE(std::string(API_VERSION).empty());
}

TEST(Constants, HttpBaseUrlStartsWithHttps) {
    EXPECT_EQ(std::string(HTTP_BASE_URL).substr(0, 8), "https://");
}

TEST(Constants, SocketHostSet) {
    EXPECT_EQ(std::string(SOCKET_HOST), "api.dbcapi.me");
}

TEST(Constants, DefaultTimeouts) {
    EXPECT_EQ(DEFAULT_TIMEOUT, 60);
    EXPECT_EQ(DEFAULT_TOKEN_TIMEOUT, 120);
}

TEST(Constants, PollsIntervalSize) {
    EXPECT_EQ(POLLS_INTERVAL.size(), 9u);
}

TEST(Constants, SocketPortRange) {
    EXPECT_EQ(SOCKET_PORT_MIN, 8123);
    EXPECT_EQ(SOCKET_PORT_MAX, 8130);
    EXPECT_LT(SOCKET_PORT_MIN, SOCKET_PORT_MAX);
}

// ── HttpClient mock — upload with no file (token type) ───────────────────────
class MinimalHttpMock : public HttpClient {
public:
    using HttpClient::HttpClient;
    void set_response(const std::string& r) { resp_ = r; }
protected:
    std::string http_call(std::string_view, std::string_view,
                          const Params&,
                          const std::vector<std::pair<std::string,
                              std::vector<std::uint8_t>>>&) override {
        return resp_;
    }
private:
    std::string resp_;
};

TEST(CoverageBoost, UploadTokenType12TurnstileParams) {
    MinimalHttpMock c("u", "p");
    c.is_verbose = false;
    c.set_response(R"({"captcha":300,"text":null,"is_correct":false})");
    Params p = {
        {"type","12"},
        {"turnstile_params", R"({"sitekey":"abc","pageurl":"https://ex.com"})"}
    };
    Captcha cap = c.upload(std::span<const std::uint8_t>{}, p);
    EXPECT_EQ(cap.captcha, 300LL);
}

TEST(CoverageBoost, UploadTokenType5RecaptchaV3) {
    MinimalHttpMock c("u", "p");
    c.is_verbose = false;
    c.set_response(R"({"captcha":301})");
    Params p = {
        {"type","5"},
        {"token_params", R"({"googlekey":"k","pageurl":"https://ex.com","action":"submit","min_score":"0.3"})"}
    };
    Captcha cap = c.upload(std::span<const std::uint8_t>{}, p);
    EXPECT_EQ(cap.captcha, 301LL);
}

TEST(CoverageBoost, GetTextOnSolvedReturnsString) {
    MinimalHttpMock c("u", "p");
    c.is_verbose = false;
    c.set_response(R"({"captcha":1,"text":"hello","is_correct":true})");
    auto t = c.get_text(1);
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(*t, "hello");
}

TEST(CoverageBoost, GetBalancePositive) {
    MinimalHttpMock c("u", "p");
    c.is_verbose = false;
    c.set_response(R"({"user":1,"rate":0.139,"balance":50.0,"is_banned":false})");
    double bal = c.get_balance();
    EXPECT_GT(bal, 0.0);
}

// ── HttpClient with authtoken ─────────────────────────────────────────────────
TEST(CoverageBoost, HttpClientAuthtokenGetUser) {
    MinimalHttpMock c("my_authtoken");
    c.is_verbose = false;
    c.set_response(R"({"user":5,"rate":0.139,"balance":10.0,"is_banned":false})");
    User u = c.get_user();
    EXPECT_EQ(u.user, 5LL);
}

// ── SocketClient mock — decode solved on second poll ─────────────────────────
class TestableSocketClient2 : public SocketClient {
public:
    using SocketClient::SocketClient;
    MOCK_METHOD(Params, socket_call,
        (std::string_view command, Params data), (override));
};

TEST(CoverageBoost, DecodeReturnsNulloptWhenSocketTimeout) {
    TestableSocketClient2 c("u", "p");
    c.is_verbose = false;
    using ::testing::_;
    using ::testing::Return;
    EXPECT_CALL(c, socket_call("login", _)).WillOnce(Return(Params{}));
    // upload returns pending captcha
    EXPECT_CALL(c, socket_call("upload", _))
        .WillOnce(Return(Params{{"captcha","500"},{"is_correct","false"}}));
    // poll always returns still pending (no text field)
    EXPECT_CALL(c, socket_call("captcha", _))
        .WillRepeatedly(Return(Params{{"captcha","500"},{"is_correct","false"}}));

    auto result = c.decode(std::nullopt, 1, {{"type","4"},{"token_params","{}"}});
    EXPECT_FALSE(result.has_value());
}

TEST(CoverageBoost, DecodeReturnsSolvedOnSecondPoll) {
    TestableSocketClient2 c("u", "p");
    c.is_verbose = false;
    using ::testing::_;
    using ::testing::Return;
    EXPECT_CALL(c, socket_call("login", _)).WillOnce(Return(Params{}));
    EXPECT_CALL(c, socket_call("upload", _))
        .WillOnce(Return(Params{{"captcha","600"},{"is_correct","false"}}));
    // First poll: pending; second poll: solved
    EXPECT_CALL(c, socket_call("captcha", _))
        .WillOnce(Return(Params{{"captcha","600"},{"is_correct","false"}}))
        .WillOnce(Return(Params{
            {"captcha","600"},{"text","my_token"},{"is_correct","true"}}));

    auto result = c.decode(std::nullopt, 30, {{"type","4"},{"token_params","{}"}});
    ASSERT_TRUE(result.has_value());
    EXPECT_EQ(result->captcha, 600LL);
    ASSERT_TRUE(result->text.has_value());
    EXPECT_EQ(*result->text, "my_token");
}

// ── CaptchaModel: large ids ───────────────────────────────────────────────────
TEST(CoverageBoost, CaptchaLargeId) {
    Captcha c;
    c.captcha = 9999999999LL;
    EXPECT_EQ(c.captcha, 9999999999LL);
}

// ── Params: many fields (all 21 types) ───────────────────────────────────────
TEST(CoverageBoost, ParamsSupportsAllTypeFields) {
    Params p;
    p["type"]                  = "25";
    p["token_enterprise_params"] = "{}";
    p["token_params"]          = "{}";
    p["turnstile_params"]      = "{}";
    p["waf_params"]            = "{}";
    p["geetest_params"]        = "{}";
    p["lemin_params"]          = "{}";
    p["capy_params"]           = "{}";
    p["siara_params"]          = "{}";
    p["mtcaptcha_params"]      = "{}";
    p["cutcaptcha_params"]     = "{}";
    p["friendly_params"]       = "{}";
    p["datadome_params"]       = "{}";
    p["tencent_params"]        = "{}";
    p["atb_params"]            = "{}";
    p["textcaptcha"]           = "What is 2+2?";
    EXPECT_EQ(p.size(), 16u);
}

// ─── read_file_bytes error paths ──────────────────────────────────────────────

TEST(CoverageBoost, ReadFileBytesThrowsForNonExistentFile) {
    MinimalHttpMock c("u", "p");
    c.is_verbose = false;
    EXPECT_THROW(
        c.upload(std::filesystem::path("/tmp/dbc_nonexistent_file_xyz_abc")),
        std::runtime_error);
}

TEST(CoverageBoost, ReadFileBytesThrowsForEmptyFile) {
    char tmp[] = "/tmp/dbc_empty_XXXXXX";
    int fd = mkstemp(tmp);   // creates an empty file
    ::close(fd);

    MinimalHttpMock c("u", "p");
    c.is_verbose = false;
    EXPECT_THROW(
        c.upload(std::filesystem::path(tmp)),
        std::runtime_error);
    ::unlink(tmp);
}

// ─── json_to_params: number, boolean and array/object (else) branches ─────────
// MinimalHttpMock overrides http_call() but NOT parse_response() nor
// json_to_params(), so these branches are exercised when parse_response is
// called with a JSON body that contains numbers, booleans and arrays.

TEST(CoverageBoost, JsonToParamsNumberAndBoolBranches) {
    MinimalHttpMock c("u", "p");
    c.is_verbose = false;
    // "user" is a JSON number, "is_banned" is a JSON boolean
    c.set_response(R"({"user":10,"rate":0.2,"balance":3.0,"is_banned":false})");
    User u = c.get_user();
    EXPECT_EQ(u.user, 10LL);
    EXPECT_FALSE(u.is_banned);
}

TEST(CoverageBoost, JsonToParamsArrayElseBranch) {
    MinimalHttpMock c("u", "p");
    c.is_verbose = false;
    // "tags" is a JSON array → hits the else branch (v.dump())
    // user_from_params ignores unknown keys so get_user still works
    c.set_response(
        R"({"user":1,"rate":0.1,"balance":5.0,"is_banned":false,"tags":["a","b"]})");
    User u = c.get_user();
    EXPECT_EQ(u.user, 1LL);
}

TEST(CoverageBoost, JsonToParamsNestedObjectElseBranch) {
    MinimalHttpMock c("u", "p");
    c.is_verbose = false;
    // "meta" is a JSON object → hits the else branch
    c.set_response(
        R"({"user":2,"rate":0.1,"balance":1.0,"is_banned":false,"meta":{"k":"v"}})");
    User u = c.get_user();
    EXPECT_EQ(u.user, 2LL);
}

// ─── captcha_from_params: text field populated ────────────────────────────────

TEST(CoverageBoost, CaptchaFromParamsTextFieldCovered) {
    MinimalHttpMock c("u", "p");
    c.is_verbose = false;
    c.set_response(R"({"captcha":55,"text":"some_token","is_correct":true})");
    auto t = c.get_text(55);
    ASSERT_TRUE(t.has_value());
    EXPECT_EQ(*t, "some_token");
}

// ─── HttpClient with authtoken: get_auth() authtoken branch ───────────────────

TEST(CoverageBoost, GetAuthAuthtokenBranchCovered) {
    MinimalHttpMock c("my_authtoken");
    c.is_verbose = false;
    c.set_response(R"({"user":5,"rate":0.139,"balance":10.0,"is_banned":false})");
    User u = c.get_user();
    EXPECT_EQ(u.user, 5LL);
}

