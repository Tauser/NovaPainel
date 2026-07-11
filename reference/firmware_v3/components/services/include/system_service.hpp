#pragma once

#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class SystemService final : public Service {
public:
    explicit SystemService(StateStore& store);

    const char* name() const override;
    bool init() override;

private:
    StateStore& store_;
};

}  // namespace nova
