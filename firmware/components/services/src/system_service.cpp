// NovaPainel - services/system_service.cpp
// Hardware-only: esp_system.h (esp_reset_reason) + nvs.h, neither of which
// have a host shim (see tools/scripts/host_check.sh SKIP_FILES).
#include "system_service.hpp"

#include "esp_log.h"
#include "esp_system.h"
#include "nvs.h"

namespace nova {

namespace {
constexpr const char* kTag = "SystemService";
constexpr const char* kNvsNamespace = "sysdiag";  // separate from SetupService's "novapanel" - unrelated concern

// Same mapping as the Fase 0 harness (gate15_main.c's reset_reason_str()) -
// not reinvented, just reused.
const char* reset_reason_str(esp_reset_reason_t r) {
    switch (r) {
        case ESP_RST_POWERON:   return "POWERON";
        case ESP_RST_SW:        return "SW";
        case ESP_RST_PANIC:     return "PANIC";
        case ESP_RST_INT_WDT:   return "INT_WDT";
        case ESP_RST_TASK_WDT:  return "TASK_WDT";
        case ESP_RST_WDT:       return "OTHER_WDT";
        case ESP_RST_BROWNOUT:  return "BROWNOUT";
        case ESP_RST_DEEPSLEEP: return "DEEPSLEEP";
        case ESP_RST_EXT:       return "EXT";
        default:                return "UNKNOWN";
    }
}
}  // namespace

bool SystemService::init() {
    const char* reason = reset_reason_str(esp_reset_reason());

    uint32_t reboot_count = 0;
    nvs_handle_t handle{};
    if (nvs_open(kNvsNamespace, NVS_READWRITE, &handle) == ESP_OK) {
        nvs_get_u32(handle, "reboots", &reboot_count);  // stays 0 if absent
        ++reboot_count;
        nvs_set_u32(handle, "reboots", reboot_count);
        nvs_commit(handle);
        nvs_close(handle);
    } else {
        ESP_LOGW(kTag, "nvs_open failed; reboot counter unavailable this boot");
    }

    store_.set_boot_diagnostics(reason, reboot_count);
    ESP_LOGI(kTag, "boot #%lu reset_reason=%s", static_cast<unsigned long>(reboot_count), reason);
    return true;
}

}  // namespace nova
