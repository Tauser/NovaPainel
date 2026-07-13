#pragma once

#include "app_state.hpp"
#include "i_board.hpp"
#include "state_store.hpp"

#include <cstdint>

namespace nova {

class ClockService {
public:
    ClockService(StateStore& state_store, const IBoard& board, DataSource source_when_valid);

    void tick();
    static void tick_adapter(void* context);

private:
    void maybe_start_sntp();
    void poll_sntp_sync();

    StateStore& state_store_;
    const IBoard& board_;
    DataSource source_when_valid_{DataSource::Live};
    uint64_t last_published_unix_time_s_{0};
    bool has_reported_state_{false};
    bool last_plausible_state_{false};
    bool sntp_initialized_{false};
    bool sntp_started_{false};
    bool sntp_synced_{false};
    bool sntp_start_failed_{false};
};

}  // namespace nova
