// example.Atb.cpp — type 24: ATB CAPTCHA
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    const std::string atb_params = R"({
        "proxy":      "http://user:password@127.0.0.1:1234",
        "proxytype":  "HTTP",
        "pageurl":    "https://your-atb-protected-page.example.com",
        "apiserver":  "https://cap.aisecurius.com",
        "appid":      "af23e041b22d000a11e22a230fa8991c"
    })";

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        dbc::Params params = {
            {"type",       "24"},
            {"atb_params", atb_params}
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
