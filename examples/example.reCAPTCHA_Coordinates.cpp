// example.reCAPTCHA_Coordinates.cpp — type 2: reCAPTCHA click coordinates
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        // Pass a screenshot containing the reCAPTCHA image grid
        dbc::Params params = {{"type", "2"}};
        auto result = client.decode("path/to/recaptcha_screenshot.jpg",
                                    120, params);
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
