#include "app_state.hpp"

namespace nova {

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
