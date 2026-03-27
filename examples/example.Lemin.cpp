// example.Lemin.cpp — type 14: Lemin CAPTCHA
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    const std::string lemin_params = R"({
        "proxy":     "http://user:password@127.0.0.1:1234",
        "proxytype": "HTTP",
        "captchaid": "CROPPED_099216d_8ba061383fa24ef498115023aa7189d4",
        "pageurl":   "https://dashboard.leminnow.com/auth/signup"
    })";

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        dbc::Params params = {
            {"type",          "14"},
            {"lemin_params",  lemin_params}
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
