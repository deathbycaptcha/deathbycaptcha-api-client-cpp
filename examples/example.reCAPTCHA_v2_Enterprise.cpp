// example.reCAPTCHA_v2_Enterprise.cpp — type 25: reCAPTCHA v2 Enterprise token
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    const std::string token_enterprise_params = R"({
        "proxy":     "http://user:password@127.0.0.1:1234",
        "proxytype": "HTTP",
        "googlekey": "6LcR_okUAAAAAPYr_aFBFCE2YbxbFNRvQGHr4pjv",
        "pageurl":   "https://example.com/enterprise"
    })";

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        dbc::Params params = {
            {"type",                    "25"},
            {"token_enterprise_params", token_enterprise_params}
        };
        auto result = client.decode(std::nullopt, 120, params);
        if (result) {
            std::cout << "CAPTCHA " << result->captcha
                      << " solved: " << result->text.value_or("") << '\n';
        } else {
            std::cout << "Failed to solve (timeout)\n";
        }
    } catch (const dbc::AccessDeniedException& e) {
        std::cerr << "Access denied: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
