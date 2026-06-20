#include "config.hpp"

#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "../common/hash.hpp"
#include "../common/io.hpp"
#include "../zygisk.hpp"

namespace zsc::config {
namespace {
constexpr char kConfigPath[] = "/data/adb/modules/zygisk_secure_capture/config/default.conf";
constexpr char kTargetsPath[] = "/data/adb/modules/zygisk_secure_capture/config/targets.txt";
constexpr char kExcludesPath[] = "/data/adb/modules/zygisk_secure_capture/config/exclude.txt";
constexpr size_t kLineCapacity = 512;
ConfigSnapshot g_companion_snapshot{};
pthread_once_t g_snapshot_once = PTHREAD_ONCE_INIT;

uint64_t ComputeChecksum(const ConfigSnapshot& snapshot) noexcept {
    constexpr size_t offset = offsetof(ConfigSnapshot, header) + offsetof(ConfigHeader, checksum);
    constexpr size_t end = offset + sizeof(uint64_t);
    uint64_t hash = common::HashBytes(&snapshot, offset);
    const auto* bytes = reinterpret_cast<const uint8_t*>(&snapshot);
    return common::HashBytes(bytes + end, sizeof(ConfigSnapshot) - end, hash);
}

void Finalize(ConfigSnapshot* snapshot) noexcept {
    snapshot->header.magic = kConfigMagic;
    snapshot->header.version = kConfigVersion;
    snapshot->header.header_size = sizeof(ConfigHeader);
    snapshot->header.struct_size = sizeof(ConfigSnapshot);
    snapshot->header.flags &= kKnownFeatureFlags;
    snapshot->header.reserved = 0;
    snapshot->header.checksum = 0;
    snapshot->header.checksum = ComputeChecksum(*snapshot);
}

char* Trim(char* value) noexcept {
    while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n') ++value;
    char* end = value + strlen(value);
    while (end > value && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) --end;
    *end = '\0';
    return value;
}

bool ParseBoolean(const char* value, bool fallback) noexcept {
    if (strcasecmp(value, "true") == 0 || strcasecmp(value, "yes") == 0 ||
        strcasecmp(value, "on") == 0 || strcmp(value, "1") == 0 ||
        strcasecmp(value, "auto") == 0) return true;
    if (strcasecmp(value, "false") == 0 || strcasecmp(value, "no") == 0 ||
        strcasecmp(value, "off") == 0 || strcmp(value, "0") == 0) return false;
    return fallback;
}

void SetFlag(ConfigSnapshot* snapshot, FeatureFlag flag, bool enabled) noexcept {
    if (enabled) snapshot->header.flags |= static_cast<uint32_t>(flag);
    else snapshot->header.flags &= ~static_cast<uint32_t>(flag);
}

FILE* OpenReadOnly(const char* path) noexcept {
    FILE* file = fopen(path, "re");
    return file != nullptr ? file : fopen(path, "r");
}

bool IsValidPackage(const char* text, size_t length) noexcept {
    if (length == 0 || length > 255 || text[0] == '.' || text[length - 1] == '.' ||
        text[0] == ':' || text[length - 1] == ':') return false;
    bool has_dot = false;
    for (size_t i = 0; i < length; ++i) {
        const char ch = text[i];
        const bool alpha = (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
        const bool digit = ch >= '0' && ch <= '9';
        if (!(alpha || digit || ch == '_' || ch == '.' || ch == ':')) return false;
        has_dot = has_dot || ch == '.';
    }
    return has_dot;
}

void InsertHash(uint64_t* values, uint32_t* count, uint64_t hash) noexcept {
    if (hash == 0 || *count >= kMaxPackageHashes) return;
    uint32_t position = 0;
    while (position < *count && values[position] < hash) ++position;
    if (position < *count && values[position] == hash) return;
    for (uint32_t i = *count; i > position; --i) values[i] = values[i - 1];
    values[position] = hash;
    ++(*count);
}

void ReadPackageFile(const char* path, uint64_t* values, uint32_t* count) noexcept {
    FILE* file = OpenReadOnly(path);
    if (file == nullptr) return;
    char line[kLineCapacity];
    while (fgets(line, sizeof(line), file) != nullptr) {
        if (strchr(line, '\n') == nullptr && !feof(file)) {
            int ch; do { ch = fgetc(file); } while (ch != '\n' && ch != EOF);
            continue;
        }
        char* text = Trim(line);
        if (*text == '\0' || *text == '#') continue;
        const size_t length = strlen(text);
        if (IsValidPackage(text, length)) InsertHash(values, count, common::HashAscii(text, length));
    }
    fclose(file);
}

void ReadOptions(ConfigSnapshot* snapshot) noexcept {
    FILE* file = OpenReadOnly(kConfigPath);
    if (file == nullptr) return;
    struct Option { const char* name; FeatureFlag flag; };
    constexpr Option options[] = {
        {"capture_secure_layers", kCaptureSecureLayers},
        {"metadata_sanitizer", kMetadataSanitizer},
        {"block_screenshot_detection", kScreenshotDetectionShield},
        {"block_screen_recording_detection", kRecordingDetectionShield},
        {"legacy_fallback", kLegacyRelayoutAuto},
        {"vendor_adapters", kVendorAdaptersAuto},
        {"debug", kDebugLogging},
    };
    char line[kLineCapacity];
    while (fgets(line, sizeof(line), file) != nullptr) {
        if (strchr(line, '\n') == nullptr && !feof(file)) {
            int ch; do { ch = fgetc(file); } while (ch != '\n' && ch != EOF);
            continue;
        }
        char* text = Trim(line);
        if (*text == '\0' || *text == '#') continue;
        char* separator = strchr(text, '=');
        if (separator == nullptr) continue;
        *separator = '\0';
        char* key = Trim(text);
        char* value = Trim(separator + 1);
        for (const Option& option : options) {
            if (strcmp(key, option.name) == 0) {
                SetFlag(snapshot, option.flag, ParseBoolean(value, HasFlag(*snapshot, option.flag)));
                break;
            }
        }
    }
    fclose(file);
}

bool BinaryContains(const uint64_t* values, uint32_t count, uint64_t hash) noexcept {
    uint32_t left = 0, right = count;
    while (left < right) {
        const uint32_t middle = left + (right - left) / 2;
        if (values[middle] < hash) left = middle + 1; else right = middle;
    }
    return hash != 0 && left < count && values[left] == hash;
}

void LoadSnapshot() noexcept {
    BuildDefaultSnapshot(&g_companion_snapshot);
    ReadOptions(&g_companion_snapshot);
    ReadPackageFile(kTargetsPath, g_companion_snapshot.targets, &g_companion_snapshot.header.target_count);
    ReadPackageFile(kExcludesPath, g_companion_snapshot.excludes, &g_companion_snapshot.header.exclude_count);
    Finalize(&g_companion_snapshot);
}
}  // namespace

void BuildSafeDisabledSnapshot(ConfigSnapshot* output) noexcept {
    if (output == nullptr) return;
    memset(output, 0, sizeof(*output));
    Finalize(output);
}
void BuildDefaultSnapshot(ConfigSnapshot* output) noexcept {
    if (output == nullptr) return;
    memset(output, 0, sizeof(*output));
    output->header.flags = kCaptureSecureLayers;
    Finalize(output);
}
bool ValidateSnapshot(const ConfigSnapshot& s) noexcept {
    return s.header.magic == kConfigMagic && s.header.version == kConfigVersion &&
        s.header.header_size == sizeof(ConfigHeader) && s.header.struct_size == sizeof(ConfigSnapshot) &&
        s.header.target_count <= kMaxPackageHashes && s.header.exclude_count <= kMaxPackageHashes &&
        (s.header.flags & ~kKnownFeatureFlags) == 0 && s.header.checksum == ComputeChecksum(s);
}
bool HasFlag(const ConfigSnapshot& s, FeatureFlag flag) noexcept {
    return (s.header.flags & static_cast<uint32_t>(flag)) != 0;
}
bool ContainsTarget(const ConfigSnapshot& s, uint64_t exact, uint64_t base) noexcept {
    return BinaryContains(s.targets, s.header.target_count, exact) ||
        (base != exact && BinaryContains(s.targets, s.header.target_count, base));
}
bool ContainsExclude(const ConfigSnapshot& s, uint64_t exact, uint64_t base) noexcept {
    return BinaryContains(s.excludes, s.header.exclude_count, exact) ||
        (base != exact && BinaryContains(s.excludes, s.header.exclude_count, base));
}
bool LoadFromCompanion(zygisk::Api* api, ConfigSnapshot* output) noexcept {
    if (output == nullptr) return false;
    BuildSafeDisabledSnapshot(output);
    if (api == nullptr) return false;
    const int client = api->connectCompanion();
    if (client < 0) return false;
    ConfigSnapshot received{};
    const bool ok = common::ReadFull(client, &received, sizeof(received));
    close(client);
    if (!ok || !ValidateSnapshot(received)) return false;
    memcpy(output, &received, sizeof(received));
    return true;
}
void CompanionHandler(int client) noexcept {
    if (client < 0) return;
    if (pthread_once(&g_snapshot_once, LoadSnapshot) != 0) {
        ConfigSnapshot disabled{};
        BuildSafeDisabledSnapshot(&disabled);
        (void)common::WriteFull(client, &disabled, sizeof(disabled));
        return;
    }
    (void)common::WriteFull(client, &g_companion_snapshot, sizeof(g_companion_snapshot));
}

}  // namespace zsc::config
