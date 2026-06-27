// NovaPainel - utils/result.cpp
#include "result.hpp"

namespace nova {

const char* to_string(ErrorCode code) {
    switch (code) {
        case ErrorCode::Ok:          return "Ok";
        case ErrorCode::Unknown:     return "Unknown";
        case ErrorCode::NotReady:    return "NotReady";
        case ErrorCode::Timeout:     return "Timeout";
        case ErrorCode::NotFound:    return "NotFound";
        case ErrorCode::RateLimited: return "RateLimited";
        case ErrorCode::Network:     return "Network";
        case ErrorCode::Parse:       return "Parse";
        case ErrorCode::InvalidArg:  return "InvalidArg";
    }
    return "Unknown";
}

}  // namespace nova
