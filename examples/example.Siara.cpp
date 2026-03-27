// example.Siara.cpp — type 17: Siara CAPTCHA
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    const std::string siara_params = R"({
        "proxy":      "http://user:password@127.0.0.1:1234",
        "proxytype":  "HTTP",
        "slideurlid": "OXR2LVNvCuXykkZbB8KZIfh162sNT8S2",
        "pageurl":    "https://www.cybersiara.com/book-a-demo",
        "useragent":  "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/115.0.0.0 Safari/537.36"
    })";

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        dbc::Params params = {
            {"type",          "17"},
            {"siara_params",  siara_params}
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
