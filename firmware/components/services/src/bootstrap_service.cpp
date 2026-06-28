#include "bootstrap_service.hpp"

#include "esp_timer.h"

namespace nova {

BootstrapService::BootstrapService(StateStore& store, EventBus& bus)
    : store_(store), bus_(bus) {}

const char* BootstrapService::name() const
{
    return "BootstrapService";
}

bool BootstrapService::init()
{
    store_.set_boot_state(BootState{});
    store_.set_screen(ScreenId::Boot);
    sub_id_ = bus_.subscribe([this](const Event& event) {
        if (event.type == EventType::BootSkipRequested) {
            skip_requested_ = true;
            finish_boot();
        }
    });
    return true;
}

void BootstrapService::start()
{
    started_ = true;
    boot_started_ms_ = static_cast<uint32_t>(esp_timer_get_time() / 1000);
    switched_to_home_ = false;
    skip_requested_ = false;
    store_.set_boot_state(BootState{0, BootStage::InitializingHardware});
}

BootState BootstrapService::compute_boot_state(uint32_t elapsed_ms) const
{
    const uint32_t clamped = (elapsed_ms >= kBootVisibleMs) ? kBootVisibleMs : elapsed_ms;
    const uint8_t pct = static_cast<uint8_t>((clamped * 100u) / kBootVisibleMs);

    BootStage stage = BootStage::InitializingHardware;
    if (pct >= 90) {
        stage = BootStage::Ready;
    } else if (pct >= 60) {
        stage = BootStage::VerifyingSensors;
    } else if (pct >= 30) {
        stage = BootStage::LoadingSystem;
    }

    return BootState{pct, stage};
}

void BootstrapService::finish_boot()
{
    if (switched_to_home_) return;

    store_.set_boot_state(BootState{100, BootStage::Ready});
    const bool onboarding_needed = store_.onboarding_needed();
    store_.set_screen(onboarding_needed ? ScreenId::Setup : ScreenId::Home);
    switched_to_home_ = true;
    bus_.publish(EventType::BootCompleted);
}

void BootstrapService::tick(uint32_t now_ms)
{
    if (!started_ || switched_to_home_) return;

    const uint32_t elapsed_ms = now_ms - boot_started_ms_;
    store_.set_boot_state(compute_boot_state(elapsed_ms));

    if (skip_requested_ || elapsed_ms >= kBootVisibleMs) {
        finish_boot();
    }
}

void BootstrapService::stop()
{
    if (sub_id_ != 0) {
        bus_.unsubscribe(sub_id_);
        sub_id_ = 0;
    }
}

}  // namespace nova
