// NovaPainel - cache/cache_store.cpp
// Hardware-only: esp_littlefs + the ESP-IDF VFS layer, neither of which have
// a host shim (see tools/scripts/host_check.sh SKIP_FILES).
#include "cache_store.hpp"

#include <cstdio>
#include <cstring>

#include "esp_littlefs.h"
#include "esp_log.h"

namespace nova {

namespace {
constexpr const char* kTag = "CacheStore";
constexpr const char* kBasePath = "/cache";
constexpr const char* kPartitionLabel = "storage";
constexpr uint32_t kMagic = 0x4E504331;  // 'NPC1'

#pragma pack(push, 1)
struct FileHeader {
    uint32_t magic;
    uint16_t schema_version;
    uint16_t payload_size;
};
#pragma pack(pop)

void path_for(const char* key, const char* suffix, char* out, size_t out_size) {
    std::snprintf(out, out_size, "%s/%s%s", kBasePath, key, suffix);
}
}  // namespace

bool CacheStore::mount() {
    esp_vfs_littlefs_conf_t conf = {};
    conf.base_path = kBasePath;
    conf.partition_label = kPartitionLabel;
    conf.format_if_mount_failed = true;  // self-heal a corrupt/virgin partition instead of failing forever
    conf.dont_mount = false;

    const esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "mount failed: %s", esp_err_to_name(err));
        mounted_ = false;
        return false;
    }
    mounted_ = true;
    ESP_LOGI(kTag, "mounted at %s (partition '%s')", kBasePath, kPartitionLabel);
    return true;
}

bool CacheStore::write(const char* key, uint16_t schema_version, const void* data, size_t size) {
    if (!mounted_ || size > UINT16_MAX) return false;

    char tmp_path[64];
    char final_path[64];
    path_for(key, ".tmp", tmp_path, sizeof(tmp_path));
    path_for(key, "", final_path, sizeof(final_path));

    FILE* f = std::fopen(tmp_path, "wb");
    if (!f) {
        ESP_LOGW(kTag, "write('%s'): fopen tmp failed", key);
        return false;
    }

    const FileHeader header{kMagic, schema_version, static_cast<uint16_t>(size)};
    const bool ok = std::fwrite(&header, sizeof(header), 1, f) == 1 &&
                    (size == 0 || std::fwrite(data, size, 1, f) == 1);
    std::fclose(f);

    if (!ok) {
        ESP_LOGW(kTag, "write('%s'): fwrite failed", key);
        std::remove(tmp_path);
        return false;
    }

    // Atomic on littlefs - either the old file is still there or the new
    // one is, never a half-written file (ADR-0015/0027).
    if (std::rename(tmp_path, final_path) != 0) {
        ESP_LOGW(kTag, "write('%s'): rename failed", key);
        std::remove(tmp_path);
        return false;
    }
    return true;
}

bool CacheStore::read(const char* key, uint16_t schema_version, void* data, size_t size) {
    if (!mounted_ || size > UINT16_MAX) return false;

    char final_path[64];
    path_for(key, "", final_path, sizeof(final_path));

    FILE* f = std::fopen(final_path, "rb");
    if (!f) return false;  // no cache yet - normal, not a warning

    FileHeader header{};
    const bool header_ok = std::fread(&header, sizeof(header), 1, f) == 1;
    if (!header_ok || header.magic != kMagic || header.schema_version != schema_version ||
        header.payload_size != size) {
        std::fclose(f);
        ESP_LOGI(kTag, "read('%s'): missing/corrupt/schema mismatch - treating as no cache", key);
        return false;
    }

    const bool payload_ok = (size == 0) || std::fread(data, size, 1, f) == 1;
    std::fclose(f);
    if (!payload_ok) {
        ESP_LOGW(kTag, "read('%s'): payload read failed", key);
        return false;
    }
    return true;
}

}  // namespace nova
