// NovaPainel - models/app_state.cpp
#include "app_state.hpp"

namespace nova {

const char* to_string(ScreenId id) {
    switch (id) {
        case ScreenId::Boot:       return "Boot";
        case ScreenId::Home:       return "Home";
        case ScreenId::Market:     return "Market";
        case ScreenId::Calendar:   return "Calendar";
        case ScreenId::Devices:    return "Devices";
        case ScreenId::Focus:      return "Focus";
        case ScreenId::PhotoFrame: return "PhotoFrame";
        case ScreenId::Routines:   return "Routines";
        case ScreenId::Settings:   return "Settings";
        case ScreenId::System:     return "System";
    }
    return "Unknown";
}

}  // namespace nova
