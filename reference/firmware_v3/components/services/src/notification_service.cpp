#include "notification_service.hpp"

#include <cstring>

namespace nova {

namespace {

bool is_abnormal_reset(const char* reason)
{
    if (!reason) {
        return false;
    }
    return std::strcmp(reason, "PANIC") == 0 ||
           std::strcmp(reason, "INT_WDT") == 0 ||
           std::strcmp(reason, "TASK_WDT") == 0 ||
           std::strcmp(reason, "OTHER_WDT") == 0 ||
           std::strcmp(reason, "BROWNOUT") == 0;
}

}  // namespace

NotificationService::NotificationService(StateStore& store, EventBus& bus)
    : store_(store), bus_(bus) {}

const char* NotificationService::name() const
{
    return "NotificationService";
}

bool NotificationService::init()
{
    sub_id_ = bus_.subscribe([this](const Event& event) {
        if (event.type == EventType::BootCompleted) {
            publish_boot_notification();
            publish_system_notifications();
            return;
        }

        if (event.type == EventType::SystemStatusChanged) {
            publish_system_notifications();
        }
    });
    return true;
}

void NotificationService::stop()
{
    if (sub_id_ != 0) {
        bus_.unsubscribe(sub_id_);
        sub_id_ = 0;
    }
}

void NotificationService::publish_boot_notification()
{
    if (boot_notice_sent_) {
        return;
    }

    boot_notice_sent_ = true;
    const AppState snapshot = store_.snapshot();
    NotificationItem item{};
    item.level = NotificationLevel::Success;
    item.category = NotificationCategory::System;
    item.title = "Sistema inicializado";
    item.body = snapshot.preferences.display_name.empty()
        ? "NovaPanel pronto para uso."
        : (snapshot.preferences.display_name + " pronto para uso.");
    item.timestamp_ms = snapshot.clock.last_update_ms;
    store_.push_notification(item);
}

void NotificationService::publish_system_notifications()
{
    const AppState snapshot = store_.snapshot();

    if (!abnormal_reset_notice_sent_ && is_abnormal_reset(snapshot.system.reset_reason)) {
        abnormal_reset_notice_sent_ = true;
        NotificationItem item{};
        item.level = NotificationLevel::Danger;
        item.category = NotificationCategory::System;
        item.title = "Ultimo boot teve falha";
        item.body = std::string("Reset detectado: ") + snapshot.system.reset_reason;
        item.timestamp_ms = snapshot.clock.last_update_ms;
        store_.push_notification(item);
    }

    if (!cache_notice_sent_ && !snapshot.system.cache_ready) {
        cache_notice_sent_ = true;
        NotificationItem item{};
        item.level = NotificationLevel::Warning;
        item.category = NotificationCategory::System;
        item.title = "Cache indisponivel";
        item.body = "O painel segue ativo, mas sem persistencia offline nesta sessao.";
        item.timestamp_ms = snapshot.clock.last_update_ms;
        store_.push_notification(item);
    }

    if (!network_notice_sent_ && !snapshot.system.network_ready) {
        network_notice_sent_ = true;
        NotificationItem item{};
        item.level = NotificationLevel::Info;
        item.category = NotificationCategory::Network;
        item.title = "Rede offline";
        item.body = "Dados remotos vao degradar para cache ou mock ate a conectividade voltar.";
        item.timestamp_ms = snapshot.clock.last_update_ms;
        store_.push_notification(item);
    }
}

}  // namespace nova
