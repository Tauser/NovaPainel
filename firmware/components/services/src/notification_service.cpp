// NovaPainel - services/notification_service.cpp
#include "notification_service.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "NotificationService";
}

int32_t NotificationService::notify(NotificationLevel level,
                                    NotificationCategory category,
                                    std::string title, std::string body) {
    Notification n{};
    n.id = next_id_++;
    n.level = level;
    n.category = category;
    n.title = std::move(title);
    n.body = std::move(body);
    items_.push_back(n);
    ESP_LOGI(kTag, "notify #%ld: %s", static_cast<long>(n.id), n.title.c_str());
    bus_.publish(EventType::NotificationCreated, n.id);
    return n.id;
}

}  // namespace nova
