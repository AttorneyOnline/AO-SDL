#include "moderation/RemoteDedupCache.h"

#include "utils/Crypto.h"

#include <cctype>
#include <chrono>

namespace moderation {

std::string RemoteDedupCache::normalize(std::string_view input) {
    // Light normalization: ASCII-lowercase + trim + collapse internal
    // whitespace. See the header doc for why this is intentionally less
    // aggressive than SlurFilter::normalize — we're computing an
    // equality key for an unsafe remote-API cache, so any folding step
    // that creates false-positive collisions would hand out wrong
    // verdicts for semantically distinct messages. Light normalization
    // keeps the false-hit rate essentially zero while still catching
    // the common "same sentence, different capitalization / spacing"
    // dedup targets.
    std::string out;
    out.reserve(input.size());
    bool last_was_space = true; // suppress leading whitespace
    for (char c : input) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (std::isspace(uc)) {
            if (!last_was_space)
                out.push_back(' ');
            last_was_space = true;
            continue;
        }
        if (uc < 0x80) {
            out.push_back(static_cast<char>(std::tolower(uc)));
        }
        else {
            // Non-ASCII byte: pass through unchanged. We don't fold
            // Unicode because that requires expensive case mapping and
            // the cache-miss case is cheap (just means a redundant API
            // call, which is the existing behavior).
            out.push_back(c);
        }
        last_was_space = false;
    }
    // Trailing whitespace was never written (suppressed by the next
    // non-space or by end-of-input), but if the very last char emitted
    // was a space marker, strip it.
    if (!out.empty() && out.back() == ' ')
        out.pop_back();
    return out;
}

std::string RemoteDedupCache::compute_key(std::string_view text) {
    // sha256 of the normalized form. Using the project's own crypto
    // helper instead of a raw OpenSSL call to stay decoupled from
    // libcrypto headers here.
    return crypto::sha256(normalize(text));
}

void RemoteDedupCache::configure(size_t max_entries, int ttl_seconds) {
    std::lock_guard lock(mu_);
    max_entries_ = max_entries > 0 ? max_entries : 1;
    ttl_ms_ = static_cast<int64_t>(ttl_seconds > 0 ? ttl_seconds : 1) * 1000;
}

int64_t RemoteDedupCache::now_ms() const {
    if (clock_)
        return clock_();
    return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

std::optional<RemoteClassifierResult> RemoteDedupCache::get(const std::string& key) {
    std::lock_guard lock(mu_);
    auto it = map_.find(key);
    if (it == map_.end())
        return std::nullopt;
    const int64_t t = now_ms();
    if (t - it->second.inserted_ms > ttl_ms_) {
        // Stale. Prune in place. Note we don't bother removing from
        // insertion_order_ here — the entry is gone from the map, so
        // future evictions will notice and skip the dangling order
        // entry. Keeps the common path (fresh hit, fresh miss) fast.
        map_.erase(it);
        return std::nullopt;
    }
    return it->second.result;
}

void RemoteDedupCache::put(const std::string& key, const RemoteClassifierResult& result) {
    std::lock_guard lock(mu_);
    const int64_t t = now_ms();

    auto [it, inserted] = map_.try_emplace(key, Entry{result, t});
    if (inserted) {
        insertion_order_.push_back(key);
    }
    else {
        // Refresh the stored result and timestamp. This is rare —
        // typically a put() follows a miss, so the key isn't already
        // present — but can happen if two threads race on the same
        // message. Keep the latest verdict.
        it->second.result = result;
        it->second.inserted_ms = t;
    }

    // Evict until we're at or below capacity. The insertion_order_
    // deque can contain stale keys from TTL pruning (see get()), so
    // skip any front entry whose key is already gone.
    while (map_.size() > max_entries_ && !insertion_order_.empty()) {
        auto oldest = insertion_order_.front();
        insertion_order_.pop_front();
        map_.erase(oldest);
    }
}

void RemoteDedupCache::clear() {
    std::lock_guard lock(mu_);
    map_.clear();
    insertion_order_.clear();
}

size_t RemoteDedupCache::size() const {
    std::lock_guard lock(mu_);
    return map_.size();
}

} // namespace moderation
