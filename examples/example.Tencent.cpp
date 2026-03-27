// example.Tencent.cpp — type 23: Tencent CAPTCHA
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    const std::string tencent_params = R"({
        "proxy":     "http://user:password@127.0.0.1:1234",
        "proxytype": "HTTP",
        "appid":     "your_tencent_appid_here",
        "pageurl":   "https://example.com/with-tencent-captcha"
    })";

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        dbc::Params params = {
            {"type",            "23"},
            {"tencent_params",  tencent_params}
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
