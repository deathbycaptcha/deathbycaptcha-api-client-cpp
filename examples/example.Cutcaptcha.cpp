// example.Cutcaptcha.cpp — type 19: CutCaptcha
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    const std::string cutcaptcha_params = R"({
        "proxy":      "http://user:password@127.0.0.1:1234",
        "proxytype":  "HTTP",
        "miserykey":  "56a9e9b989aa8cf99e0cea28d4b4678b84fa7a4e",
        "apikey":     "SAs61IAI",
        "pageurl":    "https://filecrypt.cc/Contact.html"
    })";

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        dbc::Params params = {
            {"type",              "19"},
            {"cutcaptcha_params", cutcaptcha_params}
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
