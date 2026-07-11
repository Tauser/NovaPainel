// NovaPainel - cache/cache_store.hpp
// Generic local cache on LittleFS (ADR-0015/0027). Knows nothing about
// WeatherSummary/MarketSummary - just versioned, atomically-written blobs by
// key, so it has no dependency on models/. Services define their own small
// "Cached*" structs and schema_version constants and pass them through here.
#pragma once

#include <cstddef>
#include <cstdint>

namespace nova {

class CacheStore {
public:
    // Mounts LittleFS on the "storage" partition (firmware/partitions.csv).
    // Call once, after board bring-up. false = no cache this session (not
    // fatal - callers just skip cache reads/writes; see app_main.cpp).
    bool mount();

    // Atomically (write to <key>.tmp, then rename) writes `size` bytes from
    // `data`, prefixed by a small header carrying `schema_version` (so a
    // future format change can be detected, even though there's no
    // migration logic yet - see ADR-0027 scope note).
    bool write(const char* key, uint16_t schema_version, const void* data, size_t size);

    // Reads back into `data` (caller-supplied buffer, exactly `size` bytes).
    // false = no cache for this key, or it's corrupt, or schema_version/size
    // don't match what the caller expects - all treated the same way (no
    // migration), the caller just has nothing to seed from.
    bool read(const char* key, uint16_t schema_version, void* data, size_t size);

private:
    void cleanup_interrupted_writes();
    bool mounted_{false};
};

}  // namespace nova
