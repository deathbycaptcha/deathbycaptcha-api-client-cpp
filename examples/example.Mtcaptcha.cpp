// example.Mtcaptcha.cpp — type 18: MTCaptcha
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    const std::string mtcaptcha_params = R"({
        "proxy":     "http://user:password@127.0.0.1:1234",
        "proxytype": "HTTP",
        "sitekey":   "your_mtcaptcha_sitekey_here",
        "pageurl":   "https://example.com/with-mtcaptcha"
    })";

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        dbc::Params params = {
            {"type",              "18"},
            {"mtcaptcha_params",  mtcaptcha_params}
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
