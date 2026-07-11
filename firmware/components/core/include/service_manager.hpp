#pragma once

#include <array>
#include <cstddef>

namespace nova {

class ServiceManager {
public:
    using TickFn = void (*)(void* context);

    bool add(TickFn tick, void* context);
    void tick();
    std::size_t service_count() const { return service_count_; }

private:
    struct Service {
        TickFn tick{nullptr};
        void* context{nullptr};
    };

    static constexpr std::size_t kMaxServices = 12;
    std::array<Service, kMaxServices> services_{};
    std::size_t service_count_{0};
};

}  // namespace nova
