// NovaPainel - services/notification_service.hpp
// Central place to raise user-facing notifications. Emits NotificationCreated on
// the EventBus. Storage/UI rendering come later; this is the contract + a small
// in-memory log. Mirrors shared/schemas/notification.schema.json.
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "event_bus.hpp"
#include "service.hpp"

namespace nova {

enum class NotificationLevel { Info, Success, Warning, Danger, Silent };
enum class NotificationCategory {
    System, Agenda, Alarm, Market, Weather, Device, Network
};

struct Notification {
    int32_t              id{0};
    NotificationLevel    level{NotificationLevel::Info};
    NotificationCategory category{NotificationCategory::System};
    std::string          title;
    std::string          body;
    bool                 read{false};
    uint32_t             created_ms{0};
};

class NotificationService : public Service {
public:
    explicit NotificationService(EventBus& bus) : bus_(bus) {}

    const char* name() const override { return "NotificationService"; }

    // Returns the id of the created notification.
    int32_t notify(NotificationLevel level, NotificationCategory category,
                   std::string title, std::string body = "");

    const std::vector<Notification>& recent() const { return items_; }

private:
    EventBus&                 bus_;
    std::vector<Notification> items_;
    int32_t                   next_id_{1};
};

}  // namespace nova
