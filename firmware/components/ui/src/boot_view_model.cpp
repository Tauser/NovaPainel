#include "boot_view_model.hpp"

#include "strings_ptbr.hpp"

namespace nova {

BootViewModel make_boot_view_model(const AppState& state) {
    if (state.system.display_ready) {
        return BootViewModel{
            strings_ptbr::kBootHeadlineReady,
            strings_ptbr::kBootDetailReady,
        };
    }

    return BootViewModel{
        strings_ptbr::kBootHeadlineRetry,
        strings_ptbr::kBootDetailRetry,
    };
}

}  // namespace nova
