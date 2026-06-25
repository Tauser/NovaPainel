// NovaPainel - services/notification_service.cpp
#include "notification_service.hpp"

#include <climits>

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "NotificationService";
}

const char* to_string(NotificationLevel level) {
    switch (level) {
        case NotificationLevel::Silent:  return "silent";
        case NotificationLevel::Info:    return "info";
        case NotificationLevel::Success: return "success";
        case NotificationLevel::Warning: return "warning";
        case NotificationLevel::Danger:  return "danger";
    }
    return "unknown";
}

const char* to_string(NotificationCategory category) {
    switch (category) {
        case NotificationCategory::System:  return "system";
        case NotificationCategory::Agenda:  return "agenda";
        case NotificationCategory::Alarm:   return "alarm";
        case NotificationCategory::Market:  return "market";
        case NotificationCategory::Weather: return "weather";
        case NotificationCategory::Device:  return "device";
        case NotificationCategory::Network: return "network";
    }
    return "unknown";
}

int32_t NotificationService::notify(NotificationLevel level,
                                    NotificationCategory category,
                                    std::string title, std::string body,
                                    uint32_t now_ms) {
    if (static_cast<int32_t>(items_.size()) >= kMaxItems) {
        // Evict the lowest-priority unread notification. If all are read,
        // drop the oldest (front) — it has served its purpose.
        auto victim = items_.end();
        int  lowest_pri = INT_MAX;
        for (auto it = items_.begin(); it != items_.end(); ++it) {
            if (it->read) continue;
            const int pri = static_cast<int>(it->level);
            if (pri < lowest_pri) { lowest_pri = pri; victim = it; }
        }
        if (victim != items_.end()) {
            ESP_LOGD(kTag, "queue full: evicting #%ld (%s)", static_cast<long>(victim->id),
                     to_string(victim->level));
            items_.erase(victim);
        } else {
            items_.erase(items_.begin());  // all read - drop oldest
        }
    }

    Notification n{};
    n.id         = next_id_++;
    n.level      = level;
    n.category   = category;
    n.title      = std::move(title);
    n.body       = std::move(body);
    n.created_ms = now_ms;
    items_.push_back(n);

    ESP_LOGI(kTag, "notify #%ld [%s/%s]: %s",
             static_cast<long>(n.id), to_string(level), to_string(category),
             n.title.c_str());
    bus_.publish(EventType::NotificationCreated, n.id);
    return n.id;
}

void NotificationService::mark_read(int32_t id) {
    for (auto& n : items_) {
        if (n.id == id) { n.read = true; return; }
    }
}

int32_t NotificationService::unread_count() const {
    int32_t count = 0;
    for (const auto& n : items_) { if (!n.read) ++count; }
    return count;
}

}  // namespace nova
