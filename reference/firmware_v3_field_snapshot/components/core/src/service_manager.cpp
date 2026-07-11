// NovaPainel - core/service_manager.cpp
#include "service_manager.hpp"

#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "ServiceManager";
}

void ServiceManager::add(Service* service) {
    if (service) services_.push_back(service);
}

bool ServiceManager::init_all() {
    bool all_ok = true;
    for (auto* s : services_) {
        const bool ok = s->init();
        ESP_LOGI(kTag, "init %-20s -> %s", s->name(), ok ? "ok" : "FAILED");
        all_ok = all_ok && ok;
    }
    return all_ok;
}

void ServiceManager::start_all() {
    for (auto* s : services_) {
        ESP_LOGI(kTag, "start %s", s->name());
        s->start();
    }
}

void ServiceManager::tick_all(uint32_t now_ms) {
    for (auto* s : services_) s->tick(now_ms);
}

void ServiceManager::stop_all() {
    for (auto* s : services_) {
        ESP_LOGI(kTag, "stop %s", s->name());
        s->stop();
    }
}

}  // namespace nova
