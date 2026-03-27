/// test_models.cpp — data model unit tests

#include <gtest/gtest.h>
#include "deathbycaptcha/deathbycaptcha.hpp"

TEST(UserModel, DefaultValues) {
    dbc::User u;
    EXPECT_EQ(u.user, 0LL);
    EXPECT_DOUBLE_EQ(u.rate, 0.0);
    EXPECT_DOUBLE_EQ(u.balance, 0.0);
    EXPECT_FALSE(u.is_banned);
}

TEST(UserModel, SetFields) {
    dbc::User u;
    u.user      = 12345;
    u.rate      = 0.139;
    u.balance   = 50.0;
    u.is_banned = false;
    EXPECT_EQ(u.user, 12345LL);
    EXPECT_NEAR(u.rate, 0.139, 1e-9);
    EXPECT_DOUBLE_EQ(u.balance, 50.0);
    EXPECT_FALSE(u.is_banned);
}

TEST(UserModel, BannedFlag) {
    dbc::User u;
    u.is_banned = true;
    EXPECT_TRUE(u.is_banned);
}

TEST(CaptchaModel, DefaultValues) {
    dbc::Captcha c;
    EXPECT_EQ(c.captcha, 0LL);
    EXPECT_FALSE(c.text.has_value());
    EXPECT_FALSE(c.is_correct);
}

TEST(CaptchaModel, WithText) {
    dbc::Captcha c;
    c.captcha    = 99887766;
    c.text       = "solved_text";
    c.is_correct = true;
    EXPECT_EQ(c.captcha, 99887766LL);
    ASSERT_TRUE(c.text.has_value());
    EXPECT_EQ(c.text.value(), "solved_text");
    EXPECT_TRUE(c.is_correct);
}

TEST(CaptchaModel, TextNullWhenEmpty) {
    dbc::Captcha c;
    c.captcha = 1;
    EXPECT_FALSE(c.text.has_value());
    EXPECT_FALSE(c.is_correct);
}

TEST(ParamsAlias, IsUnorderedMap) {
    dbc::Params p;
    p["type"]         = "4";
    p["token_params"] = R"({"googlekey":"abc","pageurl":"https://example.com"})";
    EXPECT_EQ(p.size(), 2u);
    EXPECT_EQ(p.at("type"), "4");
}
