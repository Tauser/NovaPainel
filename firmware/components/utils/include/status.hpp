#pragma once

#include <cstdint>
#include <string>
#include <utility>

namespace nova {

enum class StatusCode : uint8_t {
    Ok,
    InvalidArgument,
    Unavailable,
    RateLimited,
    Overflow,
    Truncated,
    InternalError,
};

class Status {
public:
    static Status success();
    static Status error(StatusCode code, std::string message);

    bool ok() const { return code_ == StatusCode::Ok; }
    StatusCode code() const { return code_; }
    const std::string& message() const { return message_; }

private:
    Status(StatusCode code, std::string message);

    StatusCode code_{StatusCode::Ok};
    std::string message_{};
};

template <typename T>
class Result {
public:
    static Result success(T value) { return Result(std::move(value)); }
    static Result failure(Status status) { return Result(std::move(status)); }

    bool ok() const { return status_.ok(); }
    const Status& status() const { return status_; }
    const T& value() const { return value_; }
    T& value() { return value_; }

private:
    explicit Result(T value) : status_(Status::success()), value_(std::move(value)) {}
    explicit Result(Status status) : status_(std::move(status)), value_{} {}

    Status status_;
    T value_{};
};

}  // namespace nova
