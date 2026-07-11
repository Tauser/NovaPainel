#include "status.hpp"

namespace nova {

Status Status::success() {
    return Status(StatusCode::Ok, {});
}

Status Status::error(StatusCode code, std::string message) {
    return Status(code, std::move(message));
}

Status::Status(StatusCode code, std::string message)
    : code_(code), message_(std::move(message)) {}

}  // namespace nova
