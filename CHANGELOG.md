# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [4.7.0] - 2026-03-27

### Added
- Initial public release of this repository, aligned with the 4.7.x client family.
- Official C++20 DeathByCaptcha API client repository and project layout.
- `dbc::HttpClient` — HTTPS REST client using libcurl.
- `dbc::SocketClient` — TCP socket client (cleartext, faster responses).
- Abstract `dbc::Client` base class with shared polling logic.
- `dbc::User` and `dbc::Captcha` data models.
- `dbc::AccessDeniedException` and `dbc::ServiceOverloadException` exception types.
- Support for all 21 CAPTCHA types (type IDs 0–25).
- Polling schedule `[1,1,2,3,2,2,3,2,2]` then fixed 3 s intervals.
- Multiplatform support: Linux, Windows (MSVC / MinGW), macOS.
- CMake build system with CMakePresets.json (default, release, coverage, integration, examples).
- Unit test suite using GoogleTest + GoogleMock (>= 80 % line coverage target).
- Integration test suite (skips gracefully when credentials are absent).
- GitHub Actions workflows: unit tests, coverage (self-hosted badge), API integration, publish.
- 22 example programs covering all CAPTCHA types.
- MIT license, RESPONSIBLE_USE.md, and full README.md documentation.
