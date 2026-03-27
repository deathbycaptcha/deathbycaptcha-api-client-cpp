// example.Capy.cpp — type 15: Capy CAPTCHA
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    const std::string capy_params = R"({
        "proxy":      "http://user:password@127.0.0.1:1234",
        "proxytype":  "HTTP",
        "captchakey": "PUZZLE_h4k2THJgd5dR5jYrKRHdddaSEp7aDN",
        "api_server": "https://www.capy.me/",
        "pageurl":    "https://www.capy.me/products/puzzle_captcha/"
    })";

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        dbc::Params params = {
            {"type",        "15"},
            {"capy_params", capy_params}
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
