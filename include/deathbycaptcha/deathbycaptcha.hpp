#pragma once

/**
 * @file deathbycaptcha.hpp
 * @brief DeathByCaptcha C++20 client library — public API header.
 *
 * Two concrete client classes are provided:
 *   - dbc::HttpClient  — uses HTTPS REST API (recommended for most users).
 *   - dbc::SocketClient — uses TCP socket API (faster; cleartext only).
 *
 * Both are thread-safe.  SocketClient keeps a persistent connection and
 * serialises socket operations with a mutex.
 *
 * Quick example:
 * @code
 *   dbc::HttpClient client("my_user", "my_pass");
 *   auto result = client.decode("image.jpg");
 *   if (result) {
 *       std::cout << "Solved: " << result->text.value_or("") << '\n';
 *   }
 * @endcode
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <memory>
#include <optional>
#include <span>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <vector>

// ─── DLL export macro ─────────────────────────────────────────────────────────
#if defined(_WIN32) || defined(__CYGWIN__)
#  if defined(DBC_BUILDING_DLL)
#    define DBC_API __declspec(dllexport)
#  elif defined(DBC_USING_DLL)
#    define DBC_API __declspec(dllimport)
#  else
#    define DBC_API
#  endif
#else
#  define DBC_API __attribute__((visibility("default")))
#endif

namespace dbc {

// ─── Library constants ────────────────────────────────────────────────────────

inline constexpr std::string_view API_VERSION           = "DBC/CPP v1.0.0";
inline constexpr std::string_view HTTP_BASE_URL         = "https://api.dbcapi.me/api";
inline constexpr std::string_view SOCKET_HOST           = "api.dbcapi.me";
inline constexpr int              DEFAULT_TIMEOUT        = 60;
inline constexpr int              DEFAULT_TOKEN_TIMEOUT  = 120;
inline constexpr std::array<int, 9> POLLS_INTERVAL      = {1, 1, 2, 3, 2, 2, 3, 2, 2};
inline constexpr int              DFLT_POLL_INTERVAL     = 3;
inline constexpr int              SOCKET_PORT_MIN        = 8123;
inline constexpr int              SOCKET_PORT_MAX        = 8130;

// ─── Convenience type alias ───────────────────────────────────────────────────

/// Key-value string map used for extra captcha parameters.
using Params = std::unordered_map<std::string, std::string>;

// ─── Data models ─────────────────────────────────────────────────────────────

/// Represents a DBC user account.
struct DBC_API User {
    long long user{0};
    double    rate{0.0};
    double    balance{0.0};
    bool      is_banned{false};
};

/// Represents an uploaded (and possibly solved) CAPTCHA.
struct DBC_API Captcha {
    long long                captcha{0};
    std::optional<std::string> text;        ///< Solved text; nullopt if pending.
    bool                     is_correct{false};
};

// ─── Exceptions ───────────────────────────────────────────────────────────────

/// Thrown when the server refuses authentication or the account is banned.
class DBC_API AccessDeniedException : public std::runtime_error {
public:
    explicit AccessDeniedException(const std::string& msg)
        : std::runtime_error(msg) {}
};

/// Thrown when the service is temporarily overloaded (HTTP 503).
class DBC_API ServiceOverloadException : public std::runtime_error {
public:
    explicit ServiceOverloadException(const std::string& msg)
        : std::runtime_error(msg) {}
};

// ─── Abstract base client ─────────────────────────────────────────────────────

/**
 * @brief Abstract base class for DBC API clients.
 *
 * Derive from this class only if you need a custom transport.
 * Prefer using HttpClient or SocketClient directly.
 */
class DBC_API Client {
public:
    /// Construct with username + password credentials.
    Client(std::string username, std::string password);

    /// Construct with an authtoken (obtained from the DBC control panel).
    explicit Client(std::string authtoken);

    virtual ~Client() = default;

    // ── Non-copyable, movable ────────────────────────────────────────────────
    Client(const Client&)            = delete;
    Client& operator=(const Client&) = delete;
    Client(Client&&)                 = default;
    Client& operator=(Client&&)      = default;

    // ── API surface ───────────────────────────────────────────────────────────

    /// Returns account details (id, rate, balance, is_banned).
    virtual User get_user() = 0;

    /// Returns account balance in US cents.
    virtual double get_balance();

    /// Returns details for a previously uploaded CAPTCHA.
    virtual Captcha get_captcha(long long cid) = 0;

    /// Returns the solved text for a CAPTCHA, or nullopt if still pending.
    virtual std::optional<std::string> get_text(long long cid);

    /// Reports a CAPTCHA as incorrectly solved. Returns true on success.
    virtual bool report(long long cid) = 0;

    /**
     * @brief Upload a CAPTCHA without waiting for it to be solved.
     *
     * @param file   Path to the image/audio file.
     * @param params Extra parameters (type, token_params, etc.).
     * @return Captcha object with captcha id (text will be empty until solved).
     */
    virtual Captcha upload(const std::filesystem::path& file,
                           Params params = {}) = 0;

    /**
     * @brief Upload raw bytes without waiting for it to be solved.
     *
     * @param data   File contents as a byte span.
     * @param params Extra parameters.
     */
    virtual Captcha upload(std::span<const std::uint8_t> data,
                           Params params = {}) = 0;

    /**
     * @brief Upload a CAPTCHA and poll until solved or timeout expires.
     *
     * For image/audio captchas, pass the file path via @p file.
     * For token-based captchas (type >= 4), leave @p file empty and
     * include "type" and the appropriate params field in @p params.
     *
     * @param file    Optional path to image/audio file.
     * @param timeout Seconds to wait.  0 = auto (60 for images, 120 for tokens).
     * @param params  Extra parameters (e.g. {"type","4"}, {"token_params","..."}).
     * @return Solved Captcha, or nullopt on timeout.
     */
    virtual std::optional<Captcha> decode(
        std::optional<std::filesystem::path> file = std::nullopt,
        int timeout = 0,
        Params params = {}) = 0;

    /// Release resources (socket / keep-alive connection).
    virtual void close() {}

    bool is_verbose{true};

protected:
    std::string m_username;
    std::string m_password;
    std::string m_authtoken;

    Params get_auth() const;

    /// Returns the next poll interval (seconds) and advances @p idx.
    static int get_poll_interval(std::size_t& idx);

    void log(std::string_view tag, std::string_view msg = "") const;

    /// Shared polling loop used by both concrete clients.
    std::optional<Captcha> poll_until_solved(Captcha uploaded, int timeout);
};

// ─── HttpClient ───────────────────────────────────────────────────────────────

namespace detail { struct HttpClientImpl; }

/**
 * @brief DBC HTTP/HTTPS client using libcurl.
 *
 * Thread-safe.  Each instance manages its own curl handle.
 */
class DBC_API HttpClient : public Client {
public:
    HttpClient(std::string username, std::string password);
    explicit HttpClient(std::string authtoken);
    ~HttpClient() override;

    User get_user() override;
    Captcha get_captcha(long long cid) override;
    bool report(long long cid) override;
    Captcha upload(const std::filesystem::path& file,
                   Params params = {}) override;
    Captcha upload(std::span<const std::uint8_t> data,
                   Params params = {}) override;
    std::optional<Captcha> decode(
        std::optional<std::filesystem::path> file = std::nullopt,
        int timeout = 0,
        Params params = {}) override;

protected:
    /**
     * @brief Low-level HTTP call.  Virtual so tests can inject a mock.
     *
     * @param method   "GET" or "POST".
     * @param endpoint Relative path (e.g. "captcha/123").
     * @param fields   Form fields for POST.
     * @param files    Binary files for multipart POST (name -> bytes).
     * @return Raw response body.
     */
    virtual std::string http_call(
        std::string_view method,
        std::string_view endpoint,
        const Params& fields = {},
        const std::vector<std::pair<std::string,
                                    std::vector<std::uint8_t>>>& files = {});

private:
    std::unique_ptr<detail::HttpClientImpl> m_impl;

    /// Parse JSON response body into a key→string map.  Throws on API errors.
    Params parse_response(const std::string& body, std::string_view context);
};

// ─── SocketClient ─────────────────────────────────────────────────────────────

namespace detail { struct SocketClientImpl; }

/**
 * @brief DBC TCP socket client.
 *
 * Maintains a persistent connection.  @b Warning: credentials travel in
 * cleartext.  Use on trusted networks only.  For untrusted networks,
 * prefer HttpClient.
 */
class DBC_API SocketClient : public Client {
public:
    SocketClient(std::string username, std::string password);
    explicit SocketClient(std::string authtoken);
    ~SocketClient() override;

    void close() override;

    User get_user() override;
    Captcha get_captcha(long long cid) override;
    bool report(long long cid) override;
    Captcha upload(const std::filesystem::path& file,
                   Params params = {}) override;
    Captcha upload(std::span<const std::uint8_t> data,
                   Params params = {}) override;
    std::optional<Captcha> decode(
        std::optional<std::filesystem::path> file = std::nullopt,
        int timeout = 0,
        Params params = {}) override;

protected:
    /**
     * @brief Low-level socket send/receive.  Virtual so tests can inject a mock.
     *
     * @param command Command name (login, upload, captcha, report, user).
     * @param data    JSON payload to send.
     * @return JSON response as key→value string map.
     */
    virtual Params socket_call(std::string_view command, Params data = {});

private:
    std::unique_ptr<detail::SocketClientImpl> m_impl;

    void ensure_logged_in();
};

} // namespace dbc
