#include "app_state.hpp"

namespace nova {

const char* to_string(ScreenId screen_id) {
    switch (screen_id) {
        case ScreenId::Boot:
            return "boot";
        case ScreenId::Home:
            return "home";
        case ScreenId::Placeholder:
            return "placeholder";
    }
    return "unknown";
}

const char* to_string(DataSource source) {
    switch (source) {
        case DataSource::None:
            return "none";
        case DataSource::Live:
            return "live";
        case DataSource::Cache:
            return "cache";
        case DataSource::Mock:
            return "mock";
    }
    return "unknown";
}

}  // namespace nova
