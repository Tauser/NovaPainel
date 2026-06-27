// NovaPainel - utils/result.hpp
// Minimal Result<T> / error type for explicit, exception-free error handling.
// Header-only template; see result.cpp for the ErrorCode -> string helper.
#pragma once

#include <utility>

namespace nova {

enum class ErrorCode {
    Ok = 0,
    Unknown,
    NotReady,
    Timeout,
    NotFound,
    RateLimited,
    Network,
    Parse,
    InvalidArg,
};

// Human-readable name for an ErrorCode (implemented in result.cpp).
const char* to_string(ErrorCode code);

struct Error {
    ErrorCode code{ErrorCode::Unknown};
    const char* message{""};
};

// Lightweight result holder. Not a full std::expected, just enough for the
// firmware skeleton. T must be default-constructible.
template <typename T>
class Result {
public:
    static Result ok(T value) { return Result(std::move(value)); }
    static Result fail(Error err) { return Result(err); }
    static Result fail(ErrorCode code, const char* message = "") {
        return Result(Error{code, message});
    }

    bool is_ok() const { return ok_; }
    explicit operator bool() const { return ok_; }

    // Only valid when is_ok() is true.
    const T& value() const { return value_; }
    T& value() { return value_; }

    const Error& error() const { return error_; }

private:
    explicit Result(T value) : ok_(true), value_(std::move(value)) {}
    explicit Result(Error err) : ok_(false), error_(err) {}

    bool ok_{false};
    T value_{};
    Error error_{};
};

// Specialization-free "void" result for operations that only signal success.
class Status {
public:
    static Status ok() { return Status(true, Error{ErrorCode::Ok, ""}); }
    static Status fail(Error err) { return Status(false, err); }
    static Status fail(ErrorCode code, const char* message = "") {
        return Status(false, Error{code, message});
    }

    bool is_ok() const { return ok_; }
    explicit operator bool() const { return ok_; }
    const Error& error() const { return error_; }

private:
    Status(bool ok, Error err) : ok_(ok), error_(err) {}
    bool ok_{false};
    Error error_{};
};

}  // namespace nova
