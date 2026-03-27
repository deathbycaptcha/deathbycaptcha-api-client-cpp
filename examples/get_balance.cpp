// get_balance.cpp — Check DBC account balance
#include <iostream>
#include "deathbycaptcha/deathbycaptcha.hpp"

int main() {
    const std::string username = "your_username";
    const std::string password = "your_password";

    // Alternatively, use an authtoken:
    // dbc::HttpClient client("your_authtoken");

    dbc::HttpClient client(username, password);

    try {
        double balance = client.get_balance();
        std::cout << "Balance: " << balance << " US cents\n";

        dbc::User user = client.get_user();
        std::cout << "User ID: "   << user.user      << '\n';
        std::cout << "Rate:    "   << user.rate      << " cents/captcha\n";
        std::cout << "Banned:  "   << user.is_banned << '\n';
    } catch (const dbc::AccessDeniedException& e) {
        std::cerr << "Access denied: " << e.what() << '\n';
        return 1;
    }
    return 0;
}
