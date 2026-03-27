<p align="center">
  <a href="https://github.com/deathbycaptcha/deathbycaptcha-api-client-python"><img alt="Python" src="https://img.shields.io/badge/Python-3776AB?style=for-the-badge&logo=python&logoColor=white"></a>
  <a href="https://github.com/deathbycaptcha/deathbycaptcha-api-client-nodejs"><img alt="Node.js" src="https://img.shields.io/badge/Node.js-339933?style=for-the-badge&logo=nodedotjs&logoColor=white"></a>
  <a href="https://github.com/deathbycaptcha/deathbycaptcha-api-client-dotnet"><img alt=".NET" src="https://img.shields.io/badge/.NET-512BD4?style=for-the-badge&logo=dotnet&logoColor=white"></a>
  <a href="https://github.com/deathbycaptcha/deathbycaptcha-api-client-java"><img alt="Java" src="https://img.shields.io/badge/Java-ED8B00?style=for-the-badge&logo=openjdk&logoColor=white"></a>
  <a href="https://github.com/deathbycaptcha/deathbycaptcha-api-client-php"><img alt="PHP" src="https://img.shields.io/badge/PHP-777BB4?style=for-the-badge&logo=php&logoColor=white"></a>
  <a href="https://github.com/deathbycaptcha/deathbycaptcha-api-client-perl"><img alt="Perl" src="https://img.shields.io/badge/Perl-39457E?style=for-the-badge&logo=perl&logoColor=white"></a>
  <a href="https://github.com/deathbycaptcha/deathbycaptcha-api-client-c11"><img alt="C" src="https://img.shields.io/badge/C-A8B9CC?style=for-the-badge&logo=c&logoColor=black"></a>
  <a href="https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp"><img alt="C++" src="https://img.shields.io/badge/%3E-C%2B%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white&labelColor=555555"></a>
</p>

---

![Unit Tests](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/actions/workflows/unit-tests.yml/badge.svg)
![Coverage](https://img.shields.io/endpoint?url=https://deathbycaptcha.github.io/deathbycaptcha-api-client-cpp/coverage-badge.json)
![API Integration](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/actions/workflows/integration.yml/badge.svg)

# DeathByCaptcha C++ Client

Official C++20 client library for the [DeathByCaptcha](https://deathbycaptcha.com) API.  
Supports all 21 CAPTCHA types via both the HTTPS REST API and the TCP Socket API.  
Multiplatform: **Linux**, **Windows**, **macOS**.  
Requires a C++20 compiler (GCC 12+, Clang 14+, MSVC 2022+) and CMake 3.21+.

---

## Table of Contents

1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Basic usage](#basic-usage)
4. [Method reference](#method-reference)
5. [CAPTCHA Types Quick Reference](#captcha-types-quick-reference)
6. [CAPTCHA Types Extended Reference](#captcha-types-extended-reference)
7. [Authentication](#authentication)
8. [Tests](#tests)
9. [Responsible use](#responsible-use)

---

## Introduction

`deathbycaptcha-api-client-cpp` wraps the public DeathByCaptcha API in a
C++20 header + shared/static library.  Two concrete clients are provided:

- **`dbc::HttpClient`** — uses HTTPS (libcurl).  Recommended for most use cases.
- **`dbc::SocketClient`** — uses a plain TCP socket.  Faster, lower-overhead,
  but credentials travel in cleartext — use only on trusted networks.

Both clients are **thread-safe** and expose the same API surface.

---

## Installation

### Prerequisites

| Dependency | Minimum version | Notes |
|-----------|----------------|-------|
| CMake      | 3.21           | Required |
| Ninja      | any            | Recommended generator |
| libcurl    | 7.x            | System package (`libcurl4-openssl-dev` on Ubuntu) |
| nlohmann/json | 3.11        | Auto-fetched via FetchContent if not installed |
| GTest / GMock | 1.14       | Auto-fetched for tests |

### Ubuntu / Debian

```bash
sudo apt-get install cmake ninja-build libcurl4-openssl-dev nlohmann-json3-dev
```

### macOS (Homebrew)

```bash
brew install cmake ninja curl nlohmann-json
```

### Windows (vcpkg)

```powershell
vcpkg install curl nlohmann-json
```

### Build

```bash
# Clone
git clone https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp.git
cd deathbycaptcha-api-client-cpp

# Configure & build (default preset: Debug, shared+static libs + tests)
cmake --preset default
cmake --build --preset default

# Run unit tests
ctest --preset default --output-on-failure
```

Other presets: `release`, `coverage`, `integration`, `examples`.

---

## Basic usage

```cpp
#include "deathbycaptcha/deathbycaptcha.hpp"
#include <iostream>

int main() {
    // HTTP client (username + password)
    dbc::HttpClient client("your_username", "your_password");

    // Or with an authtoken:
    // dbc::HttpClient client("your_authtoken");

    // Check balance
    std::cout << "Balance: " << client.get_balance() << " US cents\n";

    // Solve a normal image CAPTCHA
    auto result = client.decode("path/to/captcha.jpg");
    if (result) {
        std::cout << "Solved: " << result->text.value_or("") << '\n';
    }

    // Solve a reCAPTCHA v2
    dbc::Params params = {
        {"type",         "4"},
        {"token_params", R"({"googlekey":"YOUR_SITEKEY","pageurl":"https://example.com"})"}
    };
    auto token = client.decode(std::nullopt, 120, params);
    if (token) {
        std::cout << "Token: " << token->text.value_or("") << '\n';
    }

    return 0;
}
```

Link against `DeathByCaptcha::cpp` in your CMakeLists.txt:

```cmake
find_package(DeathByCaptchaCpp REQUIRED)
target_link_libraries(my_app PRIVATE DeathByCaptcha::cpp)
```

---

## Method reference

All methods are available on both `dbc::HttpClient` and `dbc::SocketClient`.

| Method | Signature | Description |
|--------|-----------|-------------|
| `get_user()` | `() -> User` | Account id, rate, balance, ban status |
| `get_balance()` | `() -> double` | Balance in US cents |
| `get_captcha(id)` | `(long long) -> Captcha` | Details for an uploaded CAPTCHA |
| `get_text(id)` | `(long long) -> optional<string>` | Solved text or `nullopt` |
| `report(id)` | `(long long) -> bool` | Report an incorrect solution |
| `upload(file, params)` | `(path, Params) -> Captcha` | Upload without waiting |
| `upload(data, params)` | `(span<uint8_t>, Params) -> Captcha` | Upload raw bytes without waiting |
| `decode(file, timeout, params)` | `(optional<path>, int, Params) -> optional<Captcha>` | Upload + poll until solved |
| `close()` | `() -> void` | Release resources |

### `dbc::User`

```cpp
struct User {
    long long user;      // Account numeric ID (0 if login failed)
    double    rate;      // CAPTCHA rate in US cents
    double    balance;   // Account balance in US cents
    bool      is_banned; // True if account is suspended
};
```

### `dbc::Captcha`

```cpp
struct Captcha {
    long long                captcha;   // CAPTCHA numeric ID
    std::optional<std::string> text;    // Solved text (nullopt = still pending)
    bool                     is_correct; // False if DBC detected a bad solve
};
```

### Exceptions

| Exception | Base | When |
|-----------|------|------|
| `dbc::AccessDeniedException` | `std::runtime_error` | Invalid credentials, banned, insufficient balance |
| `dbc::ServiceOverloadException` | `std::runtime_error` | HTTP 503 / socket `service-overload` |

---

## CAPTCHA Types Quick Reference

| type | Name | Params field |
|------|------|-------------|
| 0  | Normal Image | `captchafile` (file path / bytes) |
| 2  | reCAPTCHA Coordinates | `captchafile` (screenshot) |
| 3  | Image Group | `banner`, `banner_text`, `captchafile` |
| 4  | reCAPTCHA v2 | `token_params` (JSON) |
| 5  | reCAPTCHA v3 | `token_params` + `action`, `min_score` |
| 8  | Geetest v3 | `geetest_params` (JSON) |
| 9  | Geetest v4 | `geetest_params` (JSON) |
| 11 | Text CAPTCHA | `textcaptcha` |
| 12 | Cloudflare Turnstile | `turnstile_params` (JSON) |
| 13 | Audio CAPTCHA | file + `language` |
| 14 | Lemin | `lemin_params` (JSON) |
| 15 | Capy | `capy_params` (JSON) |
| 16 | Amazon WAF | `waf_params` (JSON) |
| 17 | Siara | `siara_params` (JSON) |
| 18 | MTCaptcha | `mtcaptcha_params` (JSON) |
| 19 | Cutcaptcha | `cutcaptcha_params` (JSON) |
| 20 | Friendly Captcha | `friendly_params` (JSON) |
| 21 | DataDome | `datadome_params` (JSON) |
| 23 | Tencent | `tencent_params` (JSON) |
| 24 | ATB | `atb_params` (JSON) |
| 25 | reCAPTCHA v2 Enterprise | `token_enterprise_params` (JSON) |

---

## CAPTCHA Types Extended Reference

### Normal Image (type 0)

```cpp
auto result = client.decode("path/to/captcha.jpg");
```

---

### reCAPTCHA v2 (type 4)

```cpp
dbc::Params params = {
    {"type", "4"},
    {"token_params", R"({"proxy":"http://u:p@host:port","proxytype":"HTTP",
        "googlekey":"YOUR_SITEKEY","pageurl":"https://example.com"})"}
};
auto result = client.decode(std::nullopt, 120, params);
```

---

### reCAPTCHA v3 (type 5)

```cpp
dbc::Params params = {
    {"type", "5"},
    {"token_params", R"({"googlekey":"KEY","pageurl":"URL",
        "action":"submit","min_score":"0.3"})"}
};
```

---

### reCAPTCHA v2 Enterprise (type 25)

```cpp
dbc::Params params = {
    {"type", "25"},
    {"token_enterprise_params", R"({"googlekey":"KEY","pageurl":"URL"})"}
};
```

---

### Cloudflare Turnstile (type 12)

```cpp
dbc::Params params = {
    {"type", "12"},
    {"turnstile_params", R"({"sitekey":"KEY","pageurl":"URL"})"}
};
```

---

### Amazon WAF (type 16)

```cpp
dbc::Params params = {
    {"type", "16"},
    {"waf_params", R"({"sitekey":"KEY","pageurl":"URL","iv":"IV","context":"CTX"})"}
};
```

---

### Audio CAPTCHA (type 13)

```cpp
dbc::Params params = {{"type","13"}, {"language","en"}};
auto result = client.decode("audio.mp3", 120, params);
```

---

### Text CAPTCHA (type 11)

```cpp
dbc::Params params = {{"type","11"}, {"textcaptcha","What is 2+2?"}};
auto result = client.decode(std::nullopt, 60, params);
```

---

### Geetest v3 (type 8)

```cpp
dbc::Params params = {
    {"type", "8"},
    {"geetest_params", R"({"gt":"GT","challenge":"CHALL","pageurl":"URL"})"}
};
```

---

### Geetest v4 (type 9)

```cpp
dbc::Params params = {
    {"type", "9"},
    {"geetest_params", R"({"captchaid":"ID","pageurl":"URL"})"}
};
```

---

### Lemin (type 14)

```cpp
dbc::Params params = {
    {"type", "14"},
    {"lemin_params", R"({"captcha_id":"ID","pageurl":"URL","api_server":"https://api.leminnow.com"})"}
};
```

---

### Capy (type 15)

```cpp
dbc::Params params = {
    {"type","15"},
    {"capy_params", R"({"sitekey":"KEY","pageurl":"URL"})"}
};
```

---

### DataDome (type 21)

```cpp
dbc::Params params = {
    {"type","21"},
    {"datadome_params", R"({"pageurl":"URL","captcha_url":"CAPTCHA_URL","userAgent":"UA"})"}
};
```

---

### MTCaptcha (type 18)

```cpp
dbc::Params params = {{"type","18"},{"mtcaptcha_params",R"({"sitekey":"KEY","pageurl":"URL"})"}};
```

---

### Cutcaptcha (type 19)

```cpp
dbc::Params params = {{"type","19"},{"cutcaptcha_params",R"({"misery_key":"KEY","api_key":"KEY","pageurl":"URL"})"}};
```

---

### Friendly Captcha (type 20)

```cpp
dbc::Params params = {{"type","20"},{"friendly_params",R"({"sitekey":"KEY","pageurl":"URL"})"}};
```

---

### Siara (type 17)

```cpp
dbc::Params params = {{"type","17"},{"siara_params",R"({"sitekey":"KEY","pageurl":"URL"})"}};
```

---

### Tencent (type 23)

```cpp
dbc::Params params = {{"type","23"},{"tencent_params",R"({"app_id":"ID","pageurl":"URL"})"}};
```

---

### ATB (type 24)

```cpp
dbc::Params params = {{"type","24"},{"atb_params",R"({"pageurl":"URL","api_server":"SERVER","app_id":"ID"})"}};
```

---

### reCAPTCHA Coordinates (type 2)

```cpp
dbc::Params params = {{"type","2"}};
auto result = client.decode("screenshot.jpg", 120, params);
```

---

### Image Group (type 3)

```cpp
dbc::Params params = {
    {"type","3"},
    {"banner_text","Select all traffic lights"},
    {"banner","path/to/banner.jpg"},
    {"grid","3x3"}
};
auto result = client.decode("grid.jpg", 120, params);
```

---

## Authentication

Two authentication methods are supported:

**Username + Password:**
```cpp
dbc::HttpClient client("my_username", "my_password");
```

**Authtoken** (obtain from the [DBC control panel](https://deathbycaptcha.com)):
```cpp
dbc::HttpClient client("my_authtoken");
```

The `authtoken` takes priority over `username`/`password` when both are set.

---

## Tests

### Unit tests (no credentials required)

```bash
cmake --preset default
cmake --build --preset default
ctest --preset default --output-on-failure
```

### Coverage report

```bash
cmake --preset coverage
cmake --build --preset coverage
cd build/cov && ctest --output-on-failure
gcovr --root ../.. --filter '../../src/' --object-directory . --txt
```

### Integration tests (live API)

Requires `DBC_USERNAME` and `DBC_PASSWORD` environment variables:

```bash
export DBC_USERNAME="your_username"
export DBC_PASSWORD="your_password"
cmake --preset integration
cmake --build --preset integration
cd build/integration && ctest --output-on-failure
```

### CI badge interpretation

| Badge | Meaning |
|-------|---------|
| ![Unit Tests](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/actions/workflows/unit-tests.yml/badge.svg) | All unit tests pass on Linux (GCC 12, GCC 14) and macOS (Clang) |
| ![Coverage](https://img.shields.io/endpoint?url=https://deathbycaptcha.github.io/deathbycaptcha-api-client-cpp/coverage-badge.json) | Line coverage >= 80% measured with gcovr on the main source |
| ![API Integration](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/actions/workflows/integration.yml/badge.svg) | Live API tests pass (skipped silently when secrets are absent) |

---

## Responsible use

See [RESPONSIBLE_USE.md](RESPONSIBLE_USE.md).  
This library is provided for legitimate automation against authorized targets only.  
Do not publish credentials or use this library to bypass security controls without permission.
