// example.Audio.cpp — type 13: Audio CAPTCHA
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    dbc::HttpClient client(username, password);

    try {
        std::cout << "Balance: " << client.get_balance() << '\n';

        // The audio file is sent as base64 via the 'audio' field.
        // Pass the raw audio path; the client encodes it.
        dbc::Params params = {
            {"type",     "13"},
            {"language", "en"}     // "en", "fr", "de", "es", etc.
        };
        auto result = client.decode("path/to/audio_captcha.mp3",
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
