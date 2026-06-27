#pragma once

#include "event_bus.hpp"
#include "service.hpp"
#include "state_store.hpp"

namespace nova {

class NotificationService final : public Service {
public:
    NotificationService(StateStore& store, EventBus& bus);

    const char* name() const override;
    bool init() override;
    void stop() override;

private:
    void publish_boot_notification();
    void publish_system_notifications();

    StateStore&     store_;
    EventBus&       bus_;
    SubscriptionId  sub_id_{0};
    bool            boot_notice_sent_{false};
    bool            abnormal_reset_notice_sent_{false};
    bool            cache_notice_sent_{false};
    bool            network_notice_sent_{false};
};

}  // namespace nova
