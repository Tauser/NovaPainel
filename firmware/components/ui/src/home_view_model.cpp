#include "home_view_model.hpp"

#include "strings_ptbr.hpp"

namespace nova {

HomeViewModel make_home_view_model(const AppState& state) {
    const char* status = state.system.display_ready ? strings_ptbr::kHomeStatusReady
                                                    : strings_ptbr::kHomeStatusDegraded;
    const char* detail = state.system.network_ready ? strings_ptbr::kHomeDetailMock
                                                    : strings_ptbr::kHomeDetailNetwork;
    return HomeViewModel{
        strings_ptbr::kHomeTitle,
        status,
        detail,
    };
}

}  // namespace nova
