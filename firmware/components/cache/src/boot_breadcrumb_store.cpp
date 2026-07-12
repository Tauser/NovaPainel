#include "boot_breadcrumb_store.hpp"

#if defined(ESP_PLATFORM)
#include "nvs.h"
#include "nvs_flash.h"
#endif

namespace nova {
namespace {
constexpr const char* kNamespace = "boot";
constexpr const char* kSchemaKey = "schema_v";
constexpr const char* kDisplayBreadcrumbKey = "disp_bread";
constexpr const char* kDisplayRetryCountKey = "disp_retry";
constexpr uint32_t kSchemaVersion = 1;

#if defined(ESP_PLATFORM)
Result<BootBreadcrumb> read_from_nvs(nvs_handle_t handle) {
    BootBreadcrumb breadcrumb{};
    uint8_t display_breadcrumb = 0;
    uint32_t display_retry_count = 0;

    const esp_err_t bread_err = nvs_get_u8(handle, kDisplayBreadcrumbKey, &display_breadcrumb);
    if (bread_err != ESP_OK && bread_err != ESP_ERR_NVS_NOT_FOUND) {
        return Result<BootBreadcrumb>::failure(
            Status::error(StatusCode::Unavailable, "nvs_get_u8 failed"));
    }

    const esp_err_t retry_err = nvs_get_u32(handle, kDisplayRetryCountKey, &display_retry_count);
    if (retry_err != ESP_OK && retry_err != ESP_ERR_NVS_NOT_FOUND) {
        return Result<BootBreadcrumb>::failure(
            Status::error(StatusCode::Unavailable, "nvs_get_u32 failed"));
    }

    breadcrumb.display_breadcrumb = display_breadcrumb != 0;
    breadcrumb.display_retry_count = display_retry_count;
    return Result<BootBreadcrumb>::success(breadcrumb);
}
#endif
}  // namespace

Status BootBreadcrumbStore::init() {
#if defined(ESP_PLATFORM)
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        err = nvs_flash_erase();
        if (err != ESP_OK) {
            return Status::error(StatusCode::Unavailable, "nvs_flash_erase failed");
        }
        err = nvs_flash_init();
    }
    if (err != ESP_OK) {
        return Status::error(StatusCode::Unavailable, "nvs_flash_init failed");
    }

    nvs_handle_t handle = 0;
    err = nvs_open(kNamespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return Status::error(StatusCode::Unavailable, "nvs_open failed");
    }

    uint32_t schema = 0;
    err = nvs_get_u32(handle, kSchemaKey, &schema);
    if (err == ESP_ERR_NVS_NOT_FOUND || schema != kSchemaVersion) {
        err = nvs_set_u32(handle, kSchemaKey, kSchemaVersion);
        if (err == ESP_OK) {
            err = nvs_commit(handle);
        }
    }
    nvs_close(handle);

    if (err != ESP_OK) {
        return Status::error(StatusCode::Unavailable, "nvs schema init failed");
    }
#endif
    return Status::success();
}

Result<BootBreadcrumb> BootBreadcrumbStore::load() const {
#if defined(ESP_PLATFORM)
    nvs_handle_t handle = 0;
    const esp_err_t err = nvs_open(kNamespace, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return Result<BootBreadcrumb>::failure(
            Status::error(StatusCode::Unavailable, "nvs_open read failed"));
    }
    const Result<BootBreadcrumb> result = read_from_nvs(handle);
    nvs_close(handle);
    return result;
#else
    return Result<BootBreadcrumb>::success(host_cache_);
#endif
}

Status BootBreadcrumbStore::save(BootBreadcrumb breadcrumb) {
#if defined(ESP_PLATFORM)
    nvs_handle_t handle = 0;
    esp_err_t err = nvs_open(kNamespace, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        return Status::error(StatusCode::Unavailable, "nvs_open write failed");
    }

    const Result<BootBreadcrumb> current = read_from_nvs(handle);
    if (!current.ok()) {
        nvs_close(handle);
        return current.status();
    }
    if (current.value().display_breadcrumb == breadcrumb.display_breadcrumb &&
        current.value().display_retry_count == breadcrumb.display_retry_count) {
        nvs_close(handle);
        return Status::success();
    }

    err = nvs_set_u32(handle, kSchemaKey, kSchemaVersion);
    if (err == ESP_OK) {
        err = nvs_set_u8(handle, kDisplayBreadcrumbKey, breadcrumb.display_breadcrumb ? 1 : 0);
    }
    if (err == ESP_OK) {
        err = nvs_set_u32(handle, kDisplayRetryCountKey, breadcrumb.display_retry_count);
    }
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);

    if (err != ESP_OK) {
        return Status::error(StatusCode::Unavailable, "nvs save failed");
    }
#else
    host_cache_ = breadcrumb;
#endif
    return Status::success();
}

Status BootBreadcrumbStore::clear() {
    return save(BootBreadcrumb{});
}

}  // namespace nova
