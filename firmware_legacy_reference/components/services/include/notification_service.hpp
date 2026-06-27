// NovaPainel - services/notification_service.hpp
// Central place to raise user-facing notifications. Emits NotificationCreated on
// the EventBus. Mirrors shared/schemas/notification.schema.json.
//
// Fase 11: priority-capped in-memory queue (kMaxItems). When full, the lowest-
// priority unread notification is evicted (Silent < Info < Success < Warning <
// Danger); if all are read, the oldest is dropped. This matches the
// "drop-oldest at lowest priority" backpressure policy (ADR-0012 / ADR-0032).
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "event_bus.hpp"
#include "service.hpp"

namespace nova {

// Priority order (ascending): Silent=0 < Info=1 < Success=2 < Warning=3 < Danger=4.
// Values are used numerically for eviction comparisons - do not reorder.
enum class NotificationLevel { Silent, Info, Success, Warning, Danger };
enum class NotificationCategory {
    System, Agenda, Alarm, Market, Weather, Device, Network
};

const char* to_string(NotificationLevel level);
const char* to_string(NotificationCategory category);

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

    // Creates a notification, evicting the lowest-priority unread item if the
    // queue is at capacity. Returns the id of the created notification.
    int32_t notify(NotificationLevel level, NotificationCategory category,
                   std::string title, std::string body = "", uint32_t now_ms = 0);

    void    mark_read(int32_t id);
    int32_t unread_count() const;

    const std::vector<Notification>& recent() const { return items_; }

private:
    static constexpr int32_t kMaxItems = 32;

    EventBus&                 bus_;
    std::vector<Notification> items_;
    int32_t                   next_id_{1};
};

}  // namespace nova
