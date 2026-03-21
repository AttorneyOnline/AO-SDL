#include "ao/game/ICMessageQueue.h"

void ICMessageQueue::enqueue(ICMessage msg) {
    if (msg.is_objection()) {
        queue_.clear();
        queue_.push_back(std::move(msg));
        playing_ = false;
        ready_ = true;
        linger_ms_ = 0;
        return;
    }

    // If this message will wait in the queue, prefetch its assets
    if (playing_ && prefetch_)
        prefetch_(msg);

    queue_.push_back(std::move(msg));

    if (!playing_) {
        ready_ = true;
    }
}

bool ICMessageQueue::tick(int delta_ms, bool current_message_done) {
    if (!playing_ || queue_.empty())
        return ready_;

    if (!current_message_done)
        return false;

    // Message is done — run linger timer before auto-advancing
    linger_ms_ += delta_ms;
    if (linger_ms_ >= LINGER_DURATION_MS) {
        playing_ = false;
        ready_ = true;
        linger_ms_ = 0;
        return true;
    }

    return false;
}

std::optional<ICMessage> ICMessageQueue::next() {
    if (!ready_ || queue_.empty())
        return std::nullopt;

    ICMessage msg = std::move(queue_.front());
    queue_.pop_front();
    playing_ = true;
    ready_ = false;
    linger_ms_ = 0;
    return msg;
}
