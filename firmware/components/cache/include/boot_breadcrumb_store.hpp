#pragma once

#include "status.hpp"

#include <cstdint>

namespace nova {

struct BootBreadcrumb {
    bool display_breadcrumb{false};
    uint32_t display_retry_count{0};
};

class BootBreadcrumbStore {
public:
    Status init();
    Result<BootBreadcrumb> load() const;
    Status save(BootBreadcrumb breadcrumb);
    Status clear();

private:
    BootBreadcrumb host_cache_{};
};

}  // namespace nova
