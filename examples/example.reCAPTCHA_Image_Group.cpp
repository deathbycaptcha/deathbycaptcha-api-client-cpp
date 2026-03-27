// example.reCAPTCHA_Image_Group.cpp — type 3: reCAPTCHA image group (legacy)
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        dbc::Params params = {
            {"type",        "3"},
            {"banner_text", "Select all images with traffic lights"},
            {"banner",      "path/to/banner_image.jpg"},
            {"grid",        "3x3"}
        };
        auto result = client.decode("path/to/grid_screenshot.jpg",
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
