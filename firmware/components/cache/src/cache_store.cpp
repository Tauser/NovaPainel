#include "cache_store.hpp"

#include <cstring>

#if defined(ESP_PLATFORM)
#include <cstdio>

#include "esp_err.h"
#include "esp_littlefs.h"
#include "esp_log.h"
#endif

namespace nova {
namespace {
constexpr const char* kTag = "CacheStore";
constexpr uint32_t kMagic = 0x4E504331u;  // "NPC1"
constexpr uint16_t kBlobVersion = 1;

struct BlobHeader {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t size;
};

void copy_domain(char (&dst)[24], const char* domain) {
    std::strncpy(dst, domain, sizeof(dst) - 1);
    dst[sizeof(dst) - 1] = '\0';
}

#if defined(ESP_PLATFORM)
constexpr const char* kMountPath = "/littlefs";
constexpr const char* kPartitionLabel = "storage";

void path_for(const char* domain, char (&out)[64], const char* suffix) {
    std::snprintf(out, sizeof(out), "%s/%s.%s", kMountPath, domain, suffix);
}
#endif

}  // namespace

const CacheStore::DomainThrottle* CacheStore::find_throttle(const char* domain) const {
    for (const DomainThrottle& entry : throttle_) {
        if (entry.used && std::strncmp(entry.domain, domain, sizeof(entry.domain)) == 0) {
            return &entry;
        }
    }
    return nullptr;
}

CacheStore::DomainThrottle* CacheStore::find_or_add_throttle(const char* domain) {
    for (DomainThrottle& entry : throttle_) {
        if (entry.used && std::strncmp(entry.domain, domain, sizeof(entry.domain)) == 0) {
            return &entry;
        }
    }
    for (DomainThrottle& entry : throttle_) {
        if (!entry.used) {
            entry.used = true;
            copy_domain(entry.domain, domain);
            return &entry;
        }
    }
    return nullptr;  // capacidade de domains esgotada
}

bool CacheStore::would_throttle(const char* domain, uint64_t now_ms) const {
    const DomainThrottle* entry = find_throttle(domain);
    if (entry == nullptr || !entry->written_this_session) {
        return false;
    }
    return now_ms < entry->last_write_ms + kMinWriteIntervalMs;
}

Status CacheStore::save(const char* domain, const void* data, std::size_t size, uint64_t now_ms) {
    if (domain == nullptr || data == nullptr || size == 0) {
        return Status::error(StatusCode::InvalidArgument, "invalid cache save arguments");
    }
    if (!ready_) {
        return Status::error(StatusCode::Unavailable, "cache store not mounted");
    }
    if (would_throttle(domain, now_ms)) {
        return Status::error(StatusCode::RateLimited, "cache write throttled");
    }

    DomainThrottle* throttle = find_or_add_throttle(domain);
    if (throttle == nullptr) {
        return Status::error(StatusCode::Overflow, "cache domain capacity exhausted");
    }

#if defined(ESP_PLATFORM)
    const BlobHeader header{kMagic, kBlobVersion, 0, static_cast<uint32_t>(size)};
    char tmp_path[64];
    char final_path[64];
    path_for(domain, tmp_path, "tmp");
    path_for(domain, final_path, "bin");

    std::FILE* f = std::fopen(tmp_path, "wb");
    if (f == nullptr) {
        return Status::error(StatusCode::Unavailable, "cache tmp open failed");
    }
    const bool write_ok = std::fwrite(&header, sizeof(header), 1, f) == 1 &&
                          std::fwrite(data, size, 1, f) == 1 &&
                          std::fflush(f) == 0;
    std::fclose(f);
    if (!write_ok) {
        std::remove(tmp_path);
        return Status::error(StatusCode::Unavailable, "cache write failed");
    }

    std::remove(final_path);
    if (std::rename(tmp_path, final_path) != 0) {
        std::remove(tmp_path);
        return Status::error(StatusCode::Unavailable, "cache rename failed");
    }
    ESP_LOGI(kTag, "saved %s (%u B)", domain, static_cast<unsigned>(size));
#else
    for (HostEntry& entry : host_entries_) {
        if (entry.present && std::strncmp(entry.domain, domain, sizeof(entry.domain)) == 0) {
            entry.bytes.assign(static_cast<const uint8_t*>(data),
                                static_cast<const uint8_t*>(data) + size);
            throttle->last_write_ms = now_ms;
            throttle->written_this_session = true;
            return Status::success();
        }
    }
    for (HostEntry& entry : host_entries_) {
        if (!entry.present) {
            entry.present = true;
            copy_domain(entry.domain, domain);
            entry.bytes.assign(static_cast<const uint8_t*>(data),
                                static_cast<const uint8_t*>(data) + size);
            throttle->last_write_ms = now_ms;
            throttle->written_this_session = true;
            return Status::success();
        }
    }
    return Status::error(StatusCode::Overflow, "cache domain capacity exhausted");
#endif

    throttle->last_write_ms = now_ms;
    throttle->written_this_session = true;
    return Status::success();
}

Result<std::vector<uint8_t>> CacheStore::load(const char* domain, std::size_t expected_size) const {
    if (domain == nullptr) {
        return Result<std::vector<uint8_t>>::failure(
            Status::error(StatusCode::InvalidArgument, "invalid cache domain"));
    }
    if (!ready_) {
        return Result<std::vector<uint8_t>>::failure(
            Status::error(StatusCode::Unavailable, "cache store not mounted"));
    }

#if defined(ESP_PLATFORM)
    char final_path[64];
    path_for(domain, final_path, "bin");

    std::FILE* f = std::fopen(final_path, "rb");
    if (f == nullptr) {
        return Result<std::vector<uint8_t>>::failure(
            Status::error(StatusCode::Unavailable, "cache blob not found"));
    }

    BlobHeader header{};
    if (std::fread(&header, sizeof(header), 1, f) != 1 ||
        header.magic != kMagic || header.version != kBlobVersion ||
        header.size != expected_size) {
        std::fclose(f);
        return Result<std::vector<uint8_t>>::failure(
            Status::error(StatusCode::InvalidArgument, "cache blob schema mismatch"));
    }

    std::vector<uint8_t> bytes(expected_size);
    if (expected_size > 0 && std::fread(bytes.data(), expected_size, 1, f) != 1) {
        std::fclose(f);
        return Result<std::vector<uint8_t>>::failure(
            Status::error(StatusCode::Unavailable, "cache blob truncated"));
    }
    std::fclose(f);
    return Result<std::vector<uint8_t>>::success(std::move(bytes));
#else
    for (const HostEntry& entry : host_entries_) {
        if (entry.present && std::strncmp(entry.domain, domain, sizeof(entry.domain)) == 0) {
            if (entry.bytes.size() != expected_size) {
                return Result<std::vector<uint8_t>>::failure(
                    Status::error(StatusCode::InvalidArgument, "cache blob schema mismatch"));
            }
            return Result<std::vector<uint8_t>>::success(entry.bytes);
        }
    }
    return Result<std::vector<uint8_t>>::failure(
        Status::error(StatusCode::Unavailable, "cache blob not found"));
#endif
}

Status CacheStore::mount() {
#if defined(ESP_PLATFORM)
    esp_vfs_littlefs_conf_t conf{};
    conf.base_path = kMountPath;
    conf.partition_label = kPartitionLabel;
    conf.format_if_mount_failed = true;
    conf.dont_mount = false;

    const esp_err_t err = esp_vfs_littlefs_register(&conf);
    if (err != ESP_OK) {
        ESP_LOGW(kTag, "mount failed: %s", esp_err_to_name(err));
        ready_ = false;
        return Status::error(StatusCode::Unavailable, "littlefs mount failed");
    }
    ESP_LOGI(kTag, "littlefs ready at %s", kMountPath);
#endif
    ready_ = true;
    return Status::success();
}

}  // namespace nova
