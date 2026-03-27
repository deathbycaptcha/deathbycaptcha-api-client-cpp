// example.Normal_Captcha.cpp — type 0: normal image CAPTCHA
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    // Socket client is preferred for lower latency:
    // dbc::SocketClient client(username, password);
    dbc::HttpClient client(username, password);

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        auto result = client.decode("path/to/normal_captcha.jpg");
        if (result) {
            std::cout << "CAPTCHA " << result->captcha
                      << " solved: " << result->text.value_or("") << '\n';
            // Report if incorrect:
            // client.report(result->captcha);
        } else {
            std::cout << "Failed to solve (timeout)\n";
        }
    } catch (const dbc::AccessDeniedException& e) {
        std::cerr << "Access denied: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
