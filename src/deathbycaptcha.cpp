// deathbycaptcha.cpp — C++20 DBC client implementation
//
// Dependencies:
//   - libcurl        : HTTP/HTTPS transport
//   - nlohmann/json  : JSON serialisation / deserialisation
//
// Platform notes:
//   - Windows  : Winsock2 (ws2_32 linked via CMake)
//   - POSIX     : BSD sockets (Linux / macOS)

// ─── Platform socket headers ───────────────────────────────────────────────────
#if defined(_WIN32) || defined(__CYGWIN__)
#  define WIN32_LEAN_AND_MEAN
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#  include <winsock2.h>
#  include <ws2tcpip.h>
#  pragma comment(lib, "ws2_32.lib")
   using dbc_socket_t = SOCKET;
   inline void dbc_close_socket(dbc_socket_t s) { closesocket(s); }
   inline bool dbc_socket_valid(dbc_socket_t s) { return s != INVALID_SOCKET; }
#else
#  include <sys/socket.h>
#  include <sys/select.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <unistd.h>
#  include <cerrno>
   using dbc_socket_t = int;
#  define INVALID_SOCKET  (-1)
#  define SOCKET_ERROR    (-1)
   inline void dbc_close_socket(dbc_socket_t s) { ::close(s); }
   inline bool dbc_socket_valid(dbc_socket_t s) { return s >= 0; }
#endif

// ─── Standard library ─────────────────────────────────────────────────────────
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <iostream>
#include <mutex>
#include <random>
#include <sstream>
#include <thread>

// ─── Third-party ─────────────────────────────────────────────────────────────
#include <curl/curl.h>
#include <nlohmann/json.hpp>

// ─── Public header ────────────────────────────────────────────────────────────
#include "deathbycaptcha/deathbycaptcha.hpp"

using json = nlohmann::json;

namespace dbc {

// ═══════════════════════════════════════════════════════════════════════════════
//  Internal helpers
// ═══════════════════════════════════════════════════════════════════════════════

namespace {

// ── Base64 encode ──────────────────────────────────────────────────────────────
static const std::string BASE64_CHARS =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
    "abcdefghijklmnopqrstuvwxyz"
    "0123456789+/";

std::string base64_encode(const std::vector<std::uint8_t>& data) {
    std::string out;
    out.reserve(((data.size() + 2) / 3) * 4);
    for (std::size_t i = 0; i < data.size(); i += 3) {
        std::uint32_t v = static_cast<std::uint32_t>(data[i]) << 16;
        if (i + 1 < data.size()) v |= static_cast<std::uint32_t>(data[i+1]) << 8;
        if (i + 2 < data.size()) v |= static_cast<std::uint32_t>(data[i+2]);
        out += BASE64_CHARS[(v >> 18) & 0x3F];
        out += BASE64_CHARS[(v >> 12) & 0x3F];
        out += (i + 1 < data.size()) ? BASE64_CHARS[(v >> 6) & 0x3F] : '=';
        out += (i + 2 < data.size()) ? BASE64_CHARS[v & 0x3F] : '=';
    }
    return out;
}

// ── Read a file into bytes ─────────────────────────────────────────────────────
std::vector<std::uint8_t> read_file_bytes(const std::filesystem::path& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) throw std::runtime_error("Cannot open file: " + path.string());
    const auto size = f.tellg();
    if (size <= 0) throw std::runtime_error("File is empty: " + path.string());
    std::vector<std::uint8_t> buf(static_cast<std::size_t>(size));
    f.seekg(0);
    f.read(reinterpret_cast<char*>(buf.data()), size);
    return buf;
}

// ── Random port selection ──────────────────────────────────────────────────────
int random_socket_port() {
    static std::mt19937 rng{std::random_device{}()};
    static std::uniform_int_distribution<int> dist(SOCKET_PORT_MIN, SOCKET_PORT_MAX);
    return dist(rng);
}

// ── Build a Captcha from a JSON or flat Params map ────────────────────────────
Captcha captcha_from_params(const Params& p) {
    Captcha c;
    if (auto it = p.find("captcha"); it != p.end() && !it->second.empty()) {
        try { c.captcha = std::stoll(it->second); } catch (...) {}
    }
    if (auto it = p.find("text"); it != p.end() && !it->second.empty()) {
        c.text = it->second;
    }
    if (auto it = p.find("is_correct"); it != p.end()) {
        c.is_correct = (it->second == "1" || it->second == "true");
    }
    return c;
}

// ── Build a User from a flat Params map ───────────────────────────────────────
User user_from_params(const Params& p) {
    User u;
    if (auto it = p.find("user"); it != p.end() && !it->second.empty()) {
        try { u.user = std::stoll(it->second); } catch (...) {}
    }
    if (auto it = p.find("rate"); it != p.end() && !it->second.empty()) {
        try { u.rate = std::stod(it->second); } catch (...) {}
    }
    if (auto it = p.find("balance"); it != p.end() && !it->second.empty()) {
        try { u.balance = std::stod(it->second); } catch (...) {}
    }
    if (auto it = p.find("is_banned"); it != p.end()) {
        u.is_banned = (it->second == "1" || it->second == "true");
    }
    return u;
}

// ── Parse a JSON string into a flat Params map ────────────────────────────────
Params json_to_params(const std::string& body) {
    Params result;
    if (body.empty()) return result;
    try {
        json j = json::parse(body);
        if (!j.is_object()) return result;
        for (auto& [k, v] : j.items()) {
            if (v.is_null())         continue; // null → absent from params
            else if (v.is_string())  result[k] = v.get<std::string>();
            else if (v.is_number())  result[k] = v.dump();
            else if (v.is_boolean()) result[k] = v.get<bool>() ? "true" : "false";
            else                     result[k] = v.dump();
        }
    } catch (...) {}
    return result;
}

// ── libcurl write callback ─────────────────────────────────────────────────────
std::size_t curl_write_cb(char* ptr, std::size_t, std::size_t nmemb,
                           std::string* buf) {
    buf->append(ptr, nmemb);
    return nmemb;
}

// ── URL-encode a single field value ──────────────────────────────────────────
std::string url_encode(CURL* curl, const std::string& value) {
    char* enc = curl_easy_escape(curl, value.c_str(),
                                 static_cast<int>(value.size()));
    std::string result = enc ? enc : value;
    if (enc) curl_free(enc);
    return result;
}

} // anonymous namespace

// ═══════════════════════════════════════════════════════════════════════════════
//  Client (base)
// ═══════════════════════════════════════════════════════════════════════════════

Client::Client(std::string username, std::string password)
    : m_username(std::move(username))
    , m_password(std::move(password)) {}

Client::Client(std::string authtoken)
    : m_authtoken(std::move(authtoken)) {}

double Client::get_balance() {
    return get_user().balance;
}

std::optional<std::string> Client::get_text(long long cid) {
    return get_captcha(cid).text;
}

Params Client::get_auth() const {
    if (!m_authtoken.empty()) return {{"authtoken", m_authtoken}};
    return {{"username", m_username}, {"password", m_password}};
}

int Client::get_poll_interval(std::size_t& idx) {
    int interval = (idx < POLLS_INTERVAL.size())
                   ? POLLS_INTERVAL[idx]
                   : DFLT_POLL_INTERVAL;
    ++idx;
    return interval;
}

void Client::log(std::string_view tag, std::string_view msg) const {
    if (!is_verbose) return;
    auto t = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::cout << t << ' ' << tag;
    if (!msg.empty()) std::cout << ' ' << msg;
    std::cout << '\n';
}

std::optional<Captcha> Client::poll_until_solved(Captcha uploaded,
                                                   int timeout) {
    using clock = std::chrono::steady_clock;
    const auto deadline = clock::now() + std::chrono::seconds(timeout);
    std::size_t idx = 0;
    while (!uploaded.text && clock::now() < deadline) {
        const int intvl = get_poll_interval(idx);
        std::this_thread::sleep_for(std::chrono::seconds(intvl));
        uploaded = get_captcha(uploaded.captcha);
    }
    if (uploaded.text && uploaded.is_correct) return uploaded;
    return std::nullopt;
}

// ═══════════════════════════════════════════════════════════════════════════════
//  HttpClient — implementation detail
// ═══════════════════════════════════════════════════════════════════════════════

namespace detail {
struct HttpClientImpl {
    CURL* curl{nullptr};
    std::mutex mtx;

    HttpClientImpl() {
        curl_global_init(CURL_GLOBAL_DEFAULT); // idempotent
        curl = curl_easy_init();
        if (!curl) throw std::runtime_error("curl_easy_init() failed");
    }
    ~HttpClientImpl() {
        if (curl) {
            curl_easy_cleanup(curl);
            curl = nullptr;
        }
    }
    HttpClientImpl(const HttpClientImpl&) = delete;
    HttpClientImpl& operator=(const HttpClientImpl&) = delete;
};
} // namespace detail

HttpClient::HttpClient(std::string username, std::string password)
    : Client(std::move(username), std::move(password))
    , m_impl(std::make_unique<detail::HttpClientImpl>()) {}

HttpClient::HttpClient(std::string authtoken)
    : Client(std::move(authtoken))
    , m_impl(std::make_unique<detail::HttpClientImpl>()) {}

HttpClient::~HttpClient() = default;

// ── Core HTTP call (virtual, overridable in tests) ────────────────────────────

std::string HttpClient::http_call(
    std::string_view method,
    std::string_view endpoint,
    const Params& fields,
    const std::vector<std::pair<std::string, std::vector<std::uint8_t>>>& files)
{
    std::lock_guard lock(m_impl->mtx);
    CURL* curl = m_impl->curl;
    std::string response_body;

    std::string url = std::string(HTTP_BASE_URL);
    if (!endpoint.empty()) url += '/' + std::string(endpoint);

    curl_easy_reset(curl);
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response_body);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, API_VERSION.data());
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);   // follow 303 → GET automatically
    curl_easy_setopt(curl, CURLOPT_POSTREDIR, CURL_REDIR_POST_ALL ^ CURL_REDIR_POST_303); // 303 → GET

    // Accept header
    struct curl_slist* headers = nullptr;
    headers = curl_slist_append(headers, "Accept: application/json");
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

    // Build POST data
    curl_mime* mime = nullptr;
    std::string post_body; // lifetime must outlast curl_easy_perform

    if (method == "POST") {
        if (files.empty()) {
            // URL-encoded form
            for (auto& [k, v] : fields) {
                if (!post_body.empty()) post_body += '&';
                post_body += url_encode(curl, k) + '=' + url_encode(curl, v);
            }
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_body.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)post_body.size());
        } else {
            // Multipart form (needed for file upload)
            mime = curl_mime_init(curl);
            // fields
            for (auto& [k, v] : fields) {
                curl_mimepart* part = curl_mime_addpart(mime);
                curl_mime_name(part, k.c_str());
                curl_mime_data(part, v.c_str(), CURL_ZERO_TERMINATED);
            }
            // files
            for (auto& [name, data] : files) {
                curl_mimepart* part = curl_mime_addpart(mime);
                curl_mime_name(part, name.c_str());
                curl_mime_data(part,
                    reinterpret_cast<const char*>(data.data()),
                    data.size());
                curl_mime_type(part, "application/octet-stream");
                curl_mime_filename(part, name.c_str());
            }
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, mime);
        }
    }

    log("HTTP " + std::string(method), url);
    const CURLcode rc = curl_easy_perform(curl);

    if (headers) curl_slist_free_all(headers);
    if (mime)    curl_mime_free(mime);

    if (rc != CURLE_OK) {
        throw std::runtime_error(std::string("curl error: ") +
                                 curl_easy_strerror(rc));
    }

    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);

    if (http_code == 403) {
        throw AccessDeniedException(
            "Access denied: check your credentials and/or balance");
    }
    if (http_code == 400 || http_code == 413) {
        throw std::invalid_argument(
            "CAPTCHA rejected by service: check parameters");
    }
    if (http_code == 503) {
        throw ServiceOverloadException(
            "Service overloaded, try again later");
    }
    if (http_code >= 500) {
        throw std::runtime_error("Server error (HTTP " +
                                 std::to_string(http_code) + ")");
    }

    log("HTTP RECV", response_body);
    return response_body;
}

Params HttpClient::parse_response(const std::string& body,
                                   std::string_view context) {
    if (body.empty())
        throw std::runtime_error(
            std::string("Empty API response from ") + std::string(context));
    Params p = json_to_params(body);
    if (p.empty() && !body.empty())
        throw std::runtime_error("Invalid API response (not JSON) from " +
                                 std::string(context));
    return p;
}

// ── Public methods ────────────────────────────────────────────────────────────

User HttpClient::get_user() {
    Params auth = get_auth();
    const std::string body = http_call("POST", "user", auth);
    Params p = parse_response(body, "GET user");
    return user_from_params(p);
}

Captcha HttpClient::get_captcha(long long cid) {
    const std::string body = http_call(
        "GET", "captcha/" + std::to_string(cid));
    Params p = parse_response(body, "GET captcha");
    return captcha_from_params(p);
}

bool HttpClient::report(long long cid) {
    Params auth = get_auth();
    const std::string body = http_call(
        "POST", "captcha/" + std::to_string(cid) + "/report", auth);
    Params p = json_to_params(body);
    // Server returns the captcha object with is_correct=false on success
    if (auto it = p.find("is_correct"); it != p.end()) {
        return it->second == "false" || it->second == "0";
    }
    return !body.empty();
}

Captcha HttpClient::upload(const std::filesystem::path& file, Params params) {
    auto bytes = read_file_bytes(file);
    return upload(std::span<const std::uint8_t>(bytes), std::move(params));
}

Captcha HttpClient::upload(std::span<const std::uint8_t> data, Params params) {
    Params auth = get_auth();
    for (auto& [k, v] : auth) params[k] = v;

    std::vector<std::pair<std::string, std::vector<std::uint8_t>>> files;
    const bool is_image = !data.empty();
    if (is_image) {
        files.push_back({"captchafile",
            std::vector<std::uint8_t>(data.begin(), data.end())});
    }

    const std::string body = http_call("POST", "captcha", params, files);
    Params p = parse_response(body, "POST captcha");
    Captcha c = captcha_from_params(p);
    if (c.captcha == 0)
        throw std::runtime_error("Upload failed: server returned captcha id 0");
    return c;
}

std::optional<Captcha> HttpClient::decode(
    std::optional<std::filesystem::path> file,
    int timeout,
    Params params)
{
    if (timeout == 0)
        timeout = file.has_value() ? DEFAULT_TIMEOUT : DEFAULT_TOKEN_TIMEOUT;

    Captcha uploaded = file.has_value()
        ? upload(file.value(), params)
        : upload(std::span<const std::uint8_t>{}, params);

    return poll_until_solved(std::move(uploaded), timeout);
}

// ═══════════════════════════════════════════════════════════════════════════════
//  SocketClient — implementation detail
// ═══════════════════════════════════════════════════════════════════════════════

namespace detail {
struct SocketClientImpl {
    dbc_socket_t sock{INVALID_SOCKET};
    std::mutex   mtx;
    bool         logged_in{false};

#if defined(_WIN32) || defined(__CYGWIN__)
    SocketClientImpl() {
        WSADATA wsa{};
        WSAStartup(MAKEWORD(2, 2), &wsa);
    }
    ~SocketClientImpl() {
        close();
        WSACleanup();
    }
#else
    SocketClientImpl()  = default;
    ~SocketClientImpl() { close(); }
#endif

    SocketClientImpl(const SocketClientImpl&) = delete;
    SocketClientImpl& operator=(const SocketClientImpl&) = delete;

    void close() {
        if (dbc_socket_valid(sock)) {
            ::shutdown(sock, 2 /* SHUT_RDWR */);
            dbc_close_socket(sock);
            sock = INVALID_SOCKET;
        }
        logged_in = false;
    }

    bool connect_to_host() {
        close();
        const int port = random_socket_port();
        struct addrinfo hints{}, *res = nullptr;
        hints.ai_family   = AF_INET;
        hints.ai_socktype = SOCK_STREAM;
        const std::string port_str = std::to_string(port);

        if (::getaddrinfo(SOCKET_HOST.data(), port_str.c_str(),
                          &hints, &res) != 0) return false;

        sock = ::socket(res->ai_family, res->ai_socktype, res->ai_protocol);
        if (!dbc_socket_valid(sock)) { freeaddrinfo(res); return false; }

        if (::connect(sock, res->ai_addr,
                      static_cast<int>(res->ai_addrlen)) == SOCKET_ERROR) {
            close();
            freeaddrinfo(res);
            return false;
        }
        freeaddrinfo(res);
        return true;
    }

    // Send JSON + CRLF terminator, then receive response until CRLF.
    std::string sendrecv(const std::string& payload) {
        if (!dbc_socket_valid(sock)) {
            if (!connect_to_host())
                throw std::runtime_error("Socket: cannot connect to " +
                                         std::string(SOCKET_HOST));
        }
        const std::string wire = payload + "\r\n";
        std::size_t sent = 0;
        while (sent < wire.size()) {
            const auto n = ::send(sock,
                wire.c_str() + sent,
                wire.size() - sent, 0);
            if (n <= 0) throw std::runtime_error("Socket send failed");
            sent += static_cast<std::size_t>(n);
        }
        std::string response;
        char buf[512];
        while (true) {
            const auto n = ::recv(sock, buf, sizeof(buf) - 1, 0);
            if (n <= 0) throw std::runtime_error("Socket recv failed");
            response.append(buf, static_cast<std::size_t>(n));
            if (response.size() >= 2 &&
                response.substr(response.size() - 2) == "\r\n") {
                response.erase(response.size() - 2);
                break;
            }
        }
        return response;
    }
};
} // namespace detail

SocketClient::SocketClient(std::string username, std::string password)
    : Client(std::move(username), std::move(password))
    , m_impl(std::make_unique<detail::SocketClientImpl>()) {}

SocketClient::SocketClient(std::string authtoken)
    : Client(std::move(authtoken))
    , m_impl(std::make_unique<detail::SocketClientImpl>()) {}

SocketClient::~SocketClient() = default;

void SocketClient::close() {
    std::lock_guard lock(m_impl->mtx);
    m_impl->close();
}

// ── Socket call (virtual, overridable in tests) ───────────────────────────────

Params SocketClient::socket_call(std::string_view command, Params data) {
    std::lock_guard lock(m_impl->mtx);

    json j = json::object();
    for (auto& [k, v] : data) j[k] = v;
    j["cmd"]     = command;
    j["version"] = API_VERSION;

    const std::string request = j.dump();
    log("SOCKET SEND", request);

    std::string response;
    // Retry once on connection loss
    for (int attempt = 0; attempt < 2; ++attempt) {
        try {
            response = m_impl->sendrecv(request);
            break;
        } catch (const std::exception&) {
            m_impl->close();
            if (attempt == 1) throw;
        }
    }

    log("SOCKET RECV", response);
    Params p = json_to_params(response);

    if (auto it = p.find("error"); it != p.end() && !it->second.empty()) {
        const std::string& err = it->second;
        if (err == "not-logged-in" || err == "invalid-credentials")
            throw AccessDeniedException("Access denied: check credentials");
        if (err == "banned")
            throw AccessDeniedException("Access denied: account suspended");
        if (err == "insufficient-funds")
            throw AccessDeniedException("Insufficient balance");
        if (err == "service-overload")
            throw ServiceOverloadException("Service overloaded, retry later");
        m_impl->close();
        throw std::runtime_error("API error: " + err);
    }
    return p;
}

void SocketClient::ensure_logged_in() {
    if (!m_impl->logged_in) {
        Params auth = get_auth();
        socket_call("login", auth);
        m_impl->logged_in = true;
    }
}

// ── Public methods ────────────────────────────────────────────────────────────

User SocketClient::get_user() {
    ensure_logged_in();
    Params p = socket_call("user");
    return user_from_params(p);
}

Captcha SocketClient::get_captcha(long long cid) {
    ensure_logged_in();
    Params p = socket_call("captcha", {{"captcha", std::to_string(cid)}});
    return captcha_from_params(p);
}

bool SocketClient::report(long long cid) {
    ensure_logged_in();
    Params p = socket_call("report", {{"captcha", std::to_string(cid)}});
    if (auto it = p.find("is_correct"); it != p.end()) {
        return it->second == "false" || it->second == "0";
    }
    return true;
}

Captcha SocketClient::upload(const std::filesystem::path& file, Params params) {
    auto bytes = read_file_bytes(file);
    return upload(std::span<const std::uint8_t>(bytes), std::move(params));
}

Captcha SocketClient::upload(std::span<const std::uint8_t> data, Params params) {
    ensure_logged_in();
    if (!data.empty()) {
        params["captcha"] = "base64:" + base64_encode(
            std::vector<std::uint8_t>(data.begin(), data.end()));
    }
    Params p = socket_call("upload", std::move(params));
    Captcha c = captcha_from_params(p);
    if (c.captcha == 0)
        throw std::runtime_error("Upload failed: server returned captcha id 0");
    return c;
}

std::optional<Captcha> SocketClient::decode(
    std::optional<std::filesystem::path> file,
    int timeout,
    Params params)
{
    if (timeout == 0)
        timeout = file.has_value() ? DEFAULT_TIMEOUT : DEFAULT_TOKEN_TIMEOUT;

    Captcha uploaded = file.has_value()
        ? upload(file.value(), params)
        : upload(std::span<const std::uint8_t>{}, params);

    return poll_until_solved(std::move(uploaded), timeout);
}

} // namespace dbc
