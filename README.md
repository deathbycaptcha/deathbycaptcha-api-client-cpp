# [DeathByCaptcha](https://deathbycaptcha.com/)


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


## 📖 Introduction

The [DeathByCaptcha](https://deathbycaptcha.com) C++ client is the official library for the DeathByCaptcha **captcha solving service**. It provides a clean, well-documented C++20 interface for integrating CAPTCHA solving into automation workflows — a common requirement when you use it as a **captcha solver for web scraping**, where CAPTCHAs block access to target pages. It supports both the HTTPS API (encrypted transport — recommended when security is a priority) and the socket-based API (faster and lower latency, recommended for high-throughput production workloads).

Key features:

- 🧩 Send image, audio and modern token-based CAPTCHA types (reCAPTCHA v2/v3, Turnstile, GeeTest, etc.).
- 🔄 Unified client API across HTTP and socket transports — switching between them requires changing a single line.
- 🔐 Thread-safe by design; each client instance manages its own connection/handle internally.
- 🏗️ Multiplatform: **Linux**, **Windows**, **macOS**. Requires C++20 (GCC 12+, Clang 14+, MSVC 2022+) and CMake 3.21+.

Quick start example (HTTP):

```cpp
#include "deathbycaptcha/deathbycaptcha.hpp"
#include <iostream>

int main() {
    dbc::HttpClient client("your_username", "your_password");
    auto result = client.decode("path/to/captcha.jpg", 120);
    if (result) {
        std::cout << result->text.value_or("") << '\n';
    }
}
```

> **🚌 Transport options:** Use `dbc::HttpClient` for encrypted HTTPS communication — credentials and data travel over TLS. Use `dbc::SocketClient` for lower latency and higher throughput — it is faster but communicates over a plain TCP connection to `api.dbcapi.me` on ports `8123–8130`.

---

### Tests Status

[![Unit Tests](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/actions/workflows/unit-tests.yml/badge.svg)](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/actions/workflows/unit-tests.yml)
[![Coverage](https://img.shields.io/endpoint?url=https://deathbycaptcha.github.io/deathbycaptcha-api-client-cpp/coverage-badge.json)](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/actions/workflows/coverage.yml)
[![API Integration](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/actions/workflows/integration.yml/badge.svg)](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/actions/workflows/integration.yml)

---

## 🗂️ Index

- [Installation](#installation)
    - [Prerequisites](#prerequisites)
    - [Ubuntu / Debian](#ubuntu--debian)
    - [macOS (Homebrew)](#macos-homebrew)
    - [Windows (vcpkg)](#windows-vcpkg)
    - [Build from source](#build-from-source)
- [How to Use DBC API Clients](#how-to-use-dbc-api-clients)
    - [Common Clients' Interface](#common-clients-interface)
    - [Available Methods](#available-methods)
- [Credentials & Configuration](#credentials--configuration)
    - [Quick Setup](#quick-setup)
- [CAPTCHA Types Quick Reference & Examples](#captcha-types-quick-reference--examples)
    - [Quick Start](#quick-start)
    - [Type Reference](#type-reference)
    - [Per-Type Code Snippets](#per-type-code-snippets)
- [CAPTCHA Types Extended Reference](#captcha-types-extended-reference)
    - [reCAPTCHA Image-Based API — Deprecated (Types 2 & 3)](#recaptcha-image-based-api--deprecated-types-2--3)
    - [reCAPTCHA Token API (v2 & v3)](#recaptcha-token-api-v2--v3)
    - [reCAPTCHA v2 API FAQ](#recaptcha-v2-api-faq)
    - [What is reCAPTCHA v3?](#what-is-recaptcha-v3)
    - [reCAPTCHA v3 API FAQ](#recaptcha-v3-api-faq)
    - [Amazon WAF API (Type 16)](#amazon-waf-api-type-16)
    - [Cloudflare Turnstile API (Type 12)](#cloudflare-turnstile-api-type-12)


<a id="installation"></a>
## 🛠️ Installation

<a id="prerequisites"></a>
### Prerequisites

| Dependency | Minimum version | Notes |
|-----------|----------------|-------|
| CMake      | 3.21           | Required |
| Ninja      | any            | Recommended generator |
| libcurl    | 7.x            | System package |
| nlohmann/json | 3.11        | Auto-fetched via FetchContent if not found |
| GTest / GMock | 1.14       | Auto-fetched for tests |

<a id="ubuntu--debian"></a>
### 📦 Ubuntu / Debian

```bash
sudo apt-get install cmake ninja-build libcurl4-openssl-dev nlohmann-json3-dev
```

<a id="macos-homebrew"></a>
### 🍎 macOS (Homebrew)

```bash
brew install cmake ninja curl nlohmann-json
```

<a id="windows-vcpkg"></a>
### 🪟 Windows (vcpkg)

```powershell
vcpkg install curl nlohmann-json
```

<a id="build-from-source"></a>
### 🐙 Build from source

```bash
git clone https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp.git
cd deathbycaptcha-api-client-cpp

# Configure & build (Debug, shared+static libs + unit tests)
cmake --preset default
cmake --build --preset default

# Run unit tests
ctest --preset default --output-on-failure
```

Other presets available: `release`, `coverage`, `integration`, `examples`.

To link the library in your own project:

```cmake
find_package(DeathByCaptchaCpp REQUIRED)
target_link_libraries(my_app PRIVATE DeathByCaptcha::cpp)
```

<a id="how-to-use-dbc-api-clients"></a>
## 🚀 How to Use DBC API Clients

<a id="common-clients-interface"></a>
### 🔌 Common Clients' Interface

All clients must be instantiated with your DeathByCaptcha credentials — either *username* and *password*, or an *authtoken* (available in the DBC user panel). Replace `HttpClient` with `SocketClient` to use the socket transport instead.

```cpp
#include "deathbycaptcha/deathbycaptcha.hpp"

// Username + password (HTTPS transport — encrypted, recommended when security matters)
dbc::HttpClient client(username, password);

// Username + password (socket transport — faster, lower latency, recommended for high throughput)
// dbc::SocketClient client(username, password);

// Authtoken only
// dbc::HttpClient client("your_authtoken");
```

| Transport | Class | Best for |
|---|---|---|
| HTTPS | `dbc::HttpClient` | Encrypted TLS transport — safer for credential handling and network-sensitive environments |
| Socket | `dbc::SocketClient` | Plain TCP — faster and lower latency, recommended for high-throughput production workloads |

Both clients share the same interface. Below is a summary of every available method.

<a id="available-methods"></a>

| Method | Signature | Returns | Description |
|---|---|---|---|
| `upload()` | `upload(path, params)` | `Captcha` | Upload a CAPTCHA for solving without waiting. Pass a file path for image/audio; use `params` for token types. |
| `upload()` | `upload(span<uint8_t>, params)` | `Captcha` | Upload raw bytes without waiting. |
| `decode()` | `decode(file, timeout, params)` | `optional<Captcha>` | Upload and poll until solved or timed out. Preferred method for most integrations. |
| `get_captcha()` | `get_captcha(long long cid)` | `Captcha` | Fetch status and result of a previously uploaded CAPTCHA by its numeric ID. |
| `get_text()` | `get_text(long long cid)` | `optional<string>` | Convenience wrapper — returns only the solved text. |
| `report()` | `report(long long cid)` | `bool` | Report a CAPTCHA as incorrectly solved to request a refund. Only report genuine errors. |
| `get_balance()` | `get_balance()` | `double` | Return the current account balance in US cents. |
| `get_user()` | `get_user()` | `User` | Return full account details. |
| `close()` | `close()` | `void` | Release the underlying connection/socket (called automatically on destruction). |

### 📬 CAPTCHA Result Object

`decode()` and `upload()` return a `dbc::Captcha` struct:

```cpp
struct Captcha {
    long long                captcha;    // Numeric CAPTCHA ID assigned by DBC
    std::optional<std::string> text;     // Solved text or token (nullopt if still pending)
    bool                     is_correct; // Whether DBC considers the solution correct
};
```

```cpp
// Example result
result->captcha    // 123456789
result->text       // "03AOPBWq_..."  (or std::nullopt if pending)
result->is_correct // true
```

### 💡 Full Usage Example

```cpp
#include "deathbycaptcha/deathbycaptcha.hpp"
#include <iostream>

int main() {
    dbc::HttpClient client("your_username", "your_password");

    try {
        std::cout << "Balance: " << client.get_balance() << " US cents\n";

        auto result = client.decode("path/to/captcha.jpg", 120);
        if (result) {
            std::cout << "Solved CAPTCHA " << result->captcha
                      << ": " << result->text.value_or("") << '\n';

            // Report only if you are certain the solution is wrong:
            // client.report(result->captcha);
        } else {
            std::cout << "Timed out\n";
        }
    } catch (const dbc::AccessDeniedException& e) {
        std::cerr << "Access denied — check your credentials and/or balance\n";
        return 1;
    }
    return 0;
}
```

### Exceptions

| Exception | Base | When |
|-----------|------|------|
| `dbc::AccessDeniedException` | `std::runtime_error` | Invalid credentials, banned, or insufficient balance |
| `dbc::ServiceOverloadException` | `std::runtime_error` | HTTP 503 / socket `service-overload` |

<a id="credentials--configuration"></a>
## 🔑 Credentials & Configuration

<a id="quick-setup"></a>
### ⚡ Quick Setup

```bash
# ① Build examples preset
cmake --preset examples
cmake --build --preset examples

# ② Open any example file and add your credentials
#    (look for the your_username / your_password placeholders)
vim examples/get_balance.cpp

# ③ Run integration tests with live credentials
export DBC_USERNAME="your_username"
export DBC_PASSWORD="your_password"
cmake --preset integration
cmake --build --preset integration
ctest --test-dir build/integration --output-on-failure

# ④ Push to repo — GitHub Actions picks up secrets automatically
git push
```

<a id="captcha-types-quick-reference--examples"></a>
## 🧩 CAPTCHA Types Quick Reference & Examples

This section covers every supported CAPTCHA type, how to build and run the example programs, and ready-to-copy code snippets. Start with the Quick Start below, then use the Type Reference table to find the type you need.

<a id="quick-start"></a>
### 🏁 Quick Start

1. **📦 Build the library and examples** (see [Installation](#installation))
2. **📂 Navigate to the `examples/` directory** and open the file for the type you need:

```bash
# Build examples
cmake --preset examples
cmake --build --preset examples

# Run balance check
./build/examples/examples/get_balance

# Run a normal image CAPTCHA example
./build/examples/examples/ex_normal
```

Before running any example, open the corresponding `.cpp` file and set your credentials:

```cpp
const std::string username = "your_username";
const std::string password = "your_password";
```

<a id="type-reference"></a>
### 📋 Type Reference

The table below maps every supported type to its use case, a code snippet, and the corresponding example file in `examples/`.

| Type ID | CAPTCHA Type | Use Case | Quick Use | C++ Sample |
| --- | --- | --- | --- | --- |
| 0 | Standard Image | Basic image CAPTCHA | [snippet](#sample-type-0-standard-image) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Normal_Captcha.cpp) |
| 2 | ~~reCAPTCHA Coordinates~~ | Deprecated — do not use for new integrations | — | — |
| 3 | ~~reCAPTCHA Image Group~~ | Deprecated — do not use for new integrations | — | — |
| 4 | reCAPTCHA v2 Token | reCAPTCHA v2 token solving | [snippet](#sample-type-4-recaptcha-v2-token) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.reCAPTCHA_v2.cpp) |
| 5 | reCAPTCHA v3 Token | reCAPTCHA v3 with risk scoring | [snippet](#sample-type-5-recaptcha-v3-token) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.reCAPTCHA_v3.cpp) |
| 25 | reCAPTCHA v2 Enterprise | reCAPTCHA v2 Enterprise tokens | [snippet](#sample-type-25-recaptcha-v2-enterprise) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.reCAPTCHA_v2_Enterprise.cpp) |
| 8 | GeeTest v3 | Geetest v3 verification | [snippet](#sample-type-8-geetest-v3) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Geetest_v3.cpp) |
| 9 | GeeTest v4 | Geetest v4 verification | [snippet](#sample-type-9-geetest-v4) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Geetest_v4.cpp) |
| 11 | Text CAPTCHA | Text-based question solving | [snippet](#sample-type-11-text-captcha) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Textcaptcha.cpp) |
| 12 | Cloudflare Turnstile | Cloudflare Turnstile token | [snippet](#sample-type-12-cloudflare-turnstile) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Turnstile.cpp) |
| 13 | Audio CAPTCHA | Audio CAPTCHA solving | [snippet](#sample-type-13-audio-captcha) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Audio.cpp) |
| 14 | Lemin | Lemin CAPTCHA | [snippet](#sample-type-14-lemin) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Lemin.cpp) |
| 15 | Capy | Capy CAPTCHA | [snippet](#sample-type-15-capy) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Capy.cpp) |
| 16 | Amazon WAF | Amazon WAF verification | [snippet](#sample-type-16-amazon-waf) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Amazon_Waf.cpp) |
| 17 | Siara | Siara CAPTCHA | [snippet](#sample-type-17-siara) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Siara.cpp) |
| 18 | MTCaptcha | MTCaptcha CAPTCHA | [snippet](#sample-type-18-mtcaptcha) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Mtcaptcha.cpp) |
| 19 | Cutcaptcha | Cutcaptcha CAPTCHA | [snippet](#sample-type-19-cutcaptcha) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Cutcaptcha.cpp) |
| 20 | Friendly Captcha | Friendly Captcha | [snippet](#sample-type-20-friendly-captcha) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Friendly.cpp) |
| 21 | DataDome | Datadome verification | [snippet](#sample-type-21-datadome) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Datadome.cpp) |
| 23 | Tencent | Tencent CAPTCHA | [snippet](#sample-type-23-tencent) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Tencent.cpp) |
| 24 | ATB | ATB CAPTCHA | [snippet](#sample-type-24-atb) | [open](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Atb.cpp) |

<a id="per-type-code-snippets"></a>
### 📝 Per-Type Code Snippets

Minimal usage snippet for each supported type. Use these as a starting point and refer to the full example files in `examples/` for complete implementations including error handling.

<a id="sample-type-0-standard-image"></a>
#### 🖼️ Sample Type 0: Standard Image
Official description: [Supported CAPTCHAs](https://deathbycaptcha.com/api#supported_captchas)
Full sample: [example.Normal_Captcha.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Normal_Captcha.cpp)

```cpp
auto result = client.decode("path/to/normal_captcha.jpg", 120);
```

---

<a id="sample-type-4-recaptcha-v2-token"></a>
#### 🤖 Sample Type 4: reCAPTCHA v2 Token
Official description: [reCAPTCHA Token API (v2)](https://deathbycaptcha.com/api/newtokenrecaptcha#token-v2)
Full sample: [example.reCAPTCHA_v2.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.reCAPTCHA_v2.cpp)

```cpp
const std::string token_params = R"({
    "proxy":     "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "googlekey": "6Le-wvkSAAAAAPBMRTvw0Q4Muexq9bi0DJwx_mJ-",
    "pageurl":   "https://www.google.com/recaptcha/api2/demo"
})";

dbc::Params params = {{"type", "4"}, {"token_params", token_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-5-recaptcha-v3-token"></a>
#### 🤖 Sample Type 5: reCAPTCHA v3 Token
Official description: [reCAPTCHA v3](https://deathbycaptcha.com/api/newtokenrecaptcha#reCAPTCHAv3)
Full sample: [example.reCAPTCHA_v3.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.reCAPTCHA_v3.cpp)

```cpp
const std::string token_params = R"({
    "proxy":     "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "googlekey": "6Le-wvkSAAAAAPBMRTvw0Q4Muexq9bi0DJwx_mJ-",
    "pageurl":   "https://www.google.com/recaptcha/api2/demo",
    "action":    "verify",
    "min_score": 0.3
})";

dbc::Params params = {{"type", "5"}, {"token_params", token_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-25-recaptcha-v2-enterprise"></a>
#### 🏢 Sample Type 25: reCAPTCHA v2 Enterprise
Official description: [reCAPTCHA v2 Enterprise](https://deathbycaptcha.com/api/newtokenrecaptcha#reCAPTCHAv2Enterprise)
Full sample: [example.reCAPTCHA_v2_Enterprise.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.reCAPTCHA_v2_Enterprise.cpp)

```cpp
const std::string token_enterprise_params = R"({
    "proxy":     "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "googlekey": "6LcW4sUUAAAAAEak1SQovtOlJSMl2iUvqCUQnPaQ",
    "pageurl":   "https://target"
})";

dbc::Params params = {{"type", "25"}, {"token_enterprise_params", token_enterprise_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-8-geetest-v3"></a>
#### 🧩 Sample Type 8: GeeTest v3
Official description: [GeeTest](https://deathbycaptcha.com/api/geetest)
Full sample: [example.Geetest_v3.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Geetest_v3.cpp)

```cpp
const std::string geetest_params = R"({
    "proxy":     "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "gt":        "gt_value",
    "challenge": "challenge_value",
    "pageurl":   "https://target"
})";

dbc::Params params = {{"type", "8"}, {"geetest_params", geetest_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-9-geetest-v4"></a>
#### 🧩 Sample Type 9: GeeTest v4
Official description: [GeeTest](https://deathbycaptcha.com/api/geetest)
Full sample: [example.Geetest_v4.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Geetest_v4.cpp)

```cpp
const std::string geetest_params = R"({
    "proxy":      "http://user:password@127.0.0.1:1234",
    "proxytype":  "HTTP",
    "captcha_id": "captcha_id",
    "pageurl":    "https://target"
})";

dbc::Params params = {{"type", "9"}, {"geetest_params", geetest_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-11-text-captcha"></a>
#### 💬 Sample Type 11: Text CAPTCHA
Official description: [Text CAPTCHA](https://deathbycaptcha.com/api/textcaptcha)
Full sample: [example.Textcaptcha.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Textcaptcha.cpp)

```cpp
dbc::Params params = {{"type", "11"}, {"textcaptcha", "What is two plus two?"}};
auto result = client.decode(std::nullopt, 60, params);
```

---

<a id="sample-type-12-cloudflare-turnstile"></a>
#### ☁️ Sample Type 12: Cloudflare Turnstile
Official description: [Cloudflare Turnstile](https://deathbycaptcha.com/api/turnstile)
Full sample: [example.Turnstile.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Turnstile.cpp)

```cpp
const std::string turnstile_params = R"({
    "proxy":     "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "sitekey":   "0x4AAAAAAAGlwMzq_9z6S9Mh",
    "pageurl":   "https://clifford.io/demo/cloudflare-turnstile"
})";

dbc::Params params = {{"type", "12"}, {"turnstile_params", turnstile_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-13-audio-captcha"></a>
#### 🔊 Sample Type 13: Audio CAPTCHA
Official description: [Audio CAPTCHA](https://deathbycaptcha.com/api/audio)
Full sample: [example.Audio.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Audio.cpp)

```cpp
// The audio file is uploaded directly; the library handles encoding.
dbc::Params params = {{"type", "13"}, {"language", "en"}};
auto result = client.decode("path/to/audio_captcha.mp3", 120, params);
```

---

<a id="sample-type-14-lemin"></a>
#### 🔵 Sample Type 14: Lemin
Official description: [Lemin](https://deathbycaptcha.com/api/lemin)
Full sample: [example.Lemin.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Lemin.cpp)

```cpp
const std::string lemin_params = R"({
    "proxy":     "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "captchaid": "CROPPED_xxx",
    "pageurl":   "https://target"
})";

dbc::Params params = {{"type", "14"}, {"lemin_params", lemin_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-15-capy"></a>
#### 🏴 Sample Type 15: Capy
Official description: [Capy](https://deathbycaptcha.com/api/capy)
Full sample: [example.Capy.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Capy.cpp)

```cpp
const std::string capy_params = R"({
    "proxy":      "http://user:password@127.0.0.1:1234",
    "proxytype":  "HTTP",
    "captchakey": "PUZZLE_xxx",
    "api_server": "https://api.capy.me/",
    "pageurl":    "https://target"
})";

dbc::Params params = {{"type", "15"}, {"capy_params", capy_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-16-amazon-waf"></a>
#### 🛡️ Sample Type 16: Amazon WAF
Official description: [Amazon WAF](https://deathbycaptcha.com/api/amazonwaf)
Full sample: [example.Amazon_Waf.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Amazon_Waf.cpp)

```cpp
const std::string waf_params = R"({
    "proxy":     "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "sitekey":   "your_aws_waf_sitekey_here",
    "pageurl":   "https://your-waf-protected-page.example.com",
    "iv":        "your_iv_here",
    "context":   "your_context_here"
})";

dbc::Params params = {{"type", "16"}, {"waf_params", waf_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-17-siara"></a>
#### 🔍 Sample Type 17: Siara
Official description: [Siara](https://deathbycaptcha.com/api/siara)
Full sample: [example.Siara.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Siara.cpp)

```cpp
const std::string siara_params = R"({
    "proxy":        "http://user:password@127.0.0.1:1234",
    "proxytype":    "HTTP",
    "slideurlid":   "slide_master_url_id",
    "pageurl":      "https://target",
    "useragent":    "Mozilla/5.0"
})";

dbc::Params params = {{"type", "17"}, {"siara_params", siara_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-18-mtcaptcha"></a>
#### 🔒 Sample Type 18: MTCaptcha
Official description: [MTCaptcha](https://deathbycaptcha.com/api/mtcaptcha)
Full sample: [example.Mtcaptcha.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Mtcaptcha.cpp)

```cpp
const std::string mtcaptcha_params = R"({
    "proxy":     "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "sitekey":   "MTPublic-xxx",
    "pageurl":   "https://target"
})";

dbc::Params params = {{"type", "18"}, {"mtcaptcha_params", mtcaptcha_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-19-cutcaptcha"></a>
#### ✂️ Sample Type 19: Cutcaptcha
Official description: [Cutcaptcha](https://deathbycaptcha.com/api/cutcaptcha)
Full sample: [example.Cutcaptcha.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Cutcaptcha.cpp)

```cpp
const std::string cutcaptcha_params = R"({
    "proxy":      "http://user:password@127.0.0.1:1234",
    "proxytype":  "HTTP",
    "apikey":     "api_key",
    "miserykey":  "misery_key",
    "pageurl":    "https://target"
})";

dbc::Params params = {{"type", "19"}, {"cutcaptcha_params", cutcaptcha_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-20-friendly-captcha"></a>
#### 💚 Sample Type 20: Friendly Captcha
Official description: [Friendly Captcha](https://deathbycaptcha.com/api/friendly)
Full sample: [example.Friendly.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Friendly.cpp)

```cpp
const std::string friendly_params = R"({
    "proxy":     "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "sitekey":   "FCMG...",
    "pageurl":   "https://target"
})";

dbc::Params params = {{"type", "20"}, {"friendly_params", friendly_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-21-datadome"></a>
#### 🛡️ Sample Type 21: DataDome
Official description: [DataDome](https://deathbycaptcha.com/api/datadome)
Full sample: [example.Datadome.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Datadome.cpp)

```cpp
const std::string datadome_params = R"({
    "proxy":       "http://user:password@127.0.0.1:1234",
    "proxytype":   "HTTP",
    "pageurl":     "https://target",
    "captcha_url": "https://target/captcha"
})";

dbc::Params params = {{"type", "21"}, {"datadome_params", datadome_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-23-tencent"></a>
#### 🌐 Sample Type 23: Tencent
Official description: [Tencent](https://deathbycaptcha.com/api/tencent)
Full sample: [example.Tencent.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Tencent.cpp)

```cpp
const std::string tencent_params = R"({
    "proxy":     "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "appid":     "appid",
    "pageurl":   "https://target"
})";

dbc::Params params = {{"type", "23"}, {"tencent_params", tencent_params}};
auto result = client.decode(std::nullopt, 120, params);
```

---

<a id="sample-type-24-atb"></a>
#### 🏷️ Sample Type 24: ATB
Official description: [ATB](https://deathbycaptcha.com/api/atb)
Full sample: [example.Atb.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Atb.cpp)

```cpp
const std::string atb_params = R"({
    "proxy":      "http://user:password@127.0.0.1:1234",
    "proxytype":  "HTTP",
    "appid":      "appid",
    "apiserver":  "https://cap.aisecurius.com",
    "pageurl":    "https://target"
})";

dbc::Params params = {{"type", "24"}, {"atb_params", atb_params}};
auto result = client.decode(std::nullopt, 120, params);
```

<a id="captcha-types-extended-reference"></a>
## 📚 CAPTCHA Types Extended Reference

Full API-level documentation for selected CAPTCHA types: parameter references, payload schemas, request/response formats, token lifespans, and integration notes.

---

<a id="recaptcha-image-based-api--deprecated-types-2--3"></a>
### ⛔ reCAPTCHA Image-Based API — Deprecated (Types 2 & 3)

> ⚠️ **Deprecated.** Types 2 (Coordinates) and 3 (Image Group) are legacy image-based reCAPTCHA challenge methods that are no longer used at captcha solving. Do not use them for new integrations — use the [reCAPTCHA Token API (v2 & v3)](#recaptcha-token-api-v2--v3) instead.

---

<a id="recaptcha-token-api-v2--v3"></a>
### 🔐 reCAPTCHA Token API (v2 & v3)

The Token-based API solves reCAPTCHA challenges by returning a token you inject directly into the page form, rather than clicking images. Given a site URL and site key, DBC solves the challenge on its side and returns a token valid for one submission.

- **Token Image API**: Provided a site URL and site key, the API returns a token that you use to submit the form on the page with the reCAPTCHA challenge.

---

<a id="recaptcha-v2-api-faq"></a>
### ❓ reCAPTCHA v2 API FAQ

**What's the Token Image API URL?**
To use the Token Image API you will have to send a HTTP POST Request to <http://api.dbcapi.me/api/captcha>

**What are the POST parameters for the Token image API?**
- **`username`**: Your DBC account username
- **`password`**: Your DBC account password
- **`type`=4**: Type 4 specifies this is the reCAPTCHA v2 Token API
- **`token_params`=json(payload)**: the data to access the recaptcha challenge

json payload structure:
- **`proxy`**: your proxy url and credentials (if any). Examples:
  - <http://127.0.0.1:3128>
  - <http://user:password@127.0.0.1:3128>
- **`proxytype`**: your proxy connection protocol. Example: `HTTP`
- **`googlekey`**: the google recaptcha site key of the website with the recaptcha. Example: `6Le-wvkSAAAAAPBMRTvw0Q4Muexq9bi0DJwx_mJ-`
- **`pageurl`**: the url of the page with the recaptcha challenges. This url has to include the path in which the recaptcha is loaded.
- **`data-s`**: Only required for Google search tokens. Use the `data-s` value inside the Google search response html. Not needed for regular tokens.

The **`proxy`** parameter is optional, but strongly recommended to prevent token rejection due to IP inconsistencies.
**Note:** If **`proxy`** is provided, **`proxytype`** is required.

Full example of **`token_params`**:
```json
{
  "proxy": "http://127.0.0.1:3128",
  "proxytype": "HTTP",
  "googlekey": "6Le-wvkSAAAAAPBMRTvw0Q4Muexq9bi0DJwx_mJ-",
  "pageurl": "http://test.com/path_with_recaptcha"
}
```

**What's the response from the Token image API?**
The token comes in the `text` field of the `dbc::Captcha` result. It's valid for one use and has a 2-minute lifespan:
```
"03AOPBWq_RPO2vLzyk0h8gH0cA2X4v3tpYCPZR6Y4yxKy1s3Eo7CHZRQntxrdsaD2H0e6S..."
```

---

<a id="what-is-recaptcha-v3"></a>
### 🔎 What is reCAPTCHA v3?
This API extends the reCAPTCHA v2 Token API with two additional parameters: `action` and **minimal score (`min_score`)**.
reCAPTCHA v3 returns a score from each user, that evaluate if user is a bot or human. Lower scores near to 0 are identified as bot. The `action` parameter is additional data used to separate different captcha validations (e.g. `login`, `register`, `sales`).

---

<a id="recaptcha-v3-api-faq"></a>
### ❓ reCAPTCHA v3 API FAQ

**What is `action` in reCAPTCHA v3?**
A parameter that allows processing user actions differently on the website. Find it by inspecting the JavaScript code for `grecaptcha.execute` calls:
```javascript
grecaptcha.execute('6Lc2fhwTAAAAAGatXTzFYfvlQMI2T7B6ji8UVV_f', {action: something})
```
The API uses `"verify"` as default if `action` is not provided.

**What is `min_score` in reCAPTCHA v3 API?**
The minimal score needed for the captcha resolution. We recommend using `0.3` — scores higher than `0.3` are hard to get.

Full example of **`token_params`** for v3:
```json
{
  "proxy": "http://127.0.0.1:3128",
  "proxytype": "HTTP",
  "googlekey": "6Le-wvkSAAAAAPBMRTvw0Q4Muexq9bi0DJwx_mJ-",
  "pageurl": "http://test.com/path_with_recaptcha",
  "action": "example/action",
  "min_score": 0.3
}
```

---

<a id="amazon-waf-api-type-16"></a>
### 🛡️ Amazon WAF API (Type 16)

Amazon WAF Captcha is part of the Intelligent Threat Mitigation system within Amazon AWS. It presents image-alignment challenges that DBC solves by returning a token you set as the `aws-waf-token` cookie on the target page.

- **Official documentation:** [deathbycaptcha.com/api/amazonwaf](https://deathbycaptcha.com/api/amazonwaf)
- **C++ sample:** [examples/example.Amazon_Waf.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Amazon_Waf.cpp)

**`waf_params` payload fields:**

| Parameter | Required | Description |
|---|---|---|
| `proxy` | Optional\* | Proxy URL with credentials. E.g. `http://user:password@127.0.0.1:3128` |
| `proxytype` | Required if proxy set | Proxy protocol. Currently only `HTTP` is supported. |
| `sitekey` | Required | Amazon WAF site key found in the page's captcha script |
| `pageurl` | Required | Full URL of the page showing the Amazon WAF challenge |
| `iv` | Required | Value of the `iv` parameter found in the captcha script |
| `context` | Required | Value of the `context` parameter found in the captcha script |
| `challengejs` | Optional | URL of the `challenge.js` script |
| `captchajs` | Optional | URL of the `captcha.js` script |

Full example of `waf_params`:
```json
{
  "proxy": "http://user:password@127.0.0.1:1234",
  "proxytype": "HTTP",
  "sitekey": "AQIDAHjcYu/GjX+QlghicBgQ/7bFaQZ+m5FKCMDnO+vTbNg96AH...",
  "pageurl": "https://efw47fpad9.execute-api.us-east-1.amazonaws.com/latest",
  "iv": "CgAFRjIw2vAAABSM",
  "context": "zPT0jOl1rQlUNaldX6LUpn4D6Tl9bJ8VUQ/NrWFxPii..."
}
```

**Response:** Returns a token string valid for one use with a 1-minute lifespan. Set it as the `aws-waf-token` cookie on the target page before submitting the form.

---

<a id="cloudflare-turnstile-api-type-12"></a>
### 🌐 Cloudflare Turnstile API (Type 12)

Cloudflare Turnstile is a CAPTCHA alternative that protects pages without requiring user interaction in most cases. DBC solves it by returning a token you inject into the target form.

- **Official documentation:** [deathbycaptcha.com/api/turnstile](https://deathbycaptcha.com/api/turnstile)
- **C++ sample:** [examples/example.Turnstile.cpp](https://github.com/deathbycaptcha/deathbycaptcha-api-client-cpp/blob/master/examples/example.Turnstile.cpp)

**`turnstile_params` payload fields:**

| Field | Required | Description |
|---|---|---|
| `proxy` | Optional | Proxy URL with optional credentials |
| `proxytype` | Required if `proxy` set | Proxy connection protocol. Currently only `HTTP` is supported. |
| `sitekey` | Required | The Turnstile site key found in `data-sitekey` attribute or `turnstile.render` call |
| `pageurl` | Required | Full URL of the page hosting the Turnstile challenge |
| `action` | Optional | Value of `data-action` attribute or the `action` option passed to `turnstile.render` |

> **📌 Note:** If `proxy` is provided, `proxytype` becomes required.

**Example `turnstile_params`:**
```json
{
    "proxy": "http://user:password@127.0.0.1:1234",
    "proxytype": "HTTP",
    "sitekey": "0x4AAAAAAAGlwMzq_9z6S9Mh",
    "pageurl": "https://testsite.com/xxx-test"
}
```

**Response:** Returns a token string valid for one use with a 2-minute lifespan. Submit it via `input[name="cf-turnstile-response"]` or pass it to the callback defined in `turnstile.render` / `data-callback`.

---

## ⚖️ Responsible Use

See [RESPONSIBLE_USE.md](RESPONSIBLE_USE.md).
This library is provided for legitimate automation against authorized targets only.
Do not publish credentials or use this library to bypass security controls without permission.
