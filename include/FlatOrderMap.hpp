#pragma once

#include <vector>
#include <cstdint>
#include <cstddef>
#include <functional>
#include <utility>

template <typename Key, typename Value, typename Hash = std::hash<Key>>
class FlatOrderMap {
public:
    enum class State : uint8_t { Empty, Occupied, Deleted };

    struct Slot {
        Key key{};
        Value value{};
        State state{State::Empty};
    };

    struct Iterator {
        Slot* slot{nullptr};
        Slot* end{nullptr};

        Iterator& operator++() {
            if (slot) {
                do {
                    ++slot;
                } while (slot < end && slot->state != State::Occupied);
            }
            return *this;
        }

        bool operator==(const Iterator& other) const { return slot == other.slot; }
        bool operator!=(const Iterator& other) const { return slot != other.slot; }

        struct Proxy {
            const Key& first;
            Value& second;
            Proxy* operator->() { return this; }
        };

        Proxy operator->() const {
            return Proxy{slot->key, slot->value};
        }

        std::pair<const Key&, Value&> operator*() const {
            return {slot->key, slot->value};
        }
    };

    struct ConstIterator {
        const Slot* slot{nullptr};
        const Slot* end{nullptr};

        ConstIterator& operator++() {
            if (slot) {
                do {
                    ++slot;
                } while (slot < end && slot->state != State::Occupied);
            }
            return *this;
        }

        bool operator==(const ConstIterator& other) const { return slot == other.slot; }
        bool operator!=(const ConstIterator& other) const { return slot != other.slot; }

        struct ConstProxy {
            const Key& first;
            const Value& second;
            const ConstProxy* operator->() const { return this; }
        };

        ConstProxy operator->() const {
            return ConstProxy{slot->key, slot->value};
        }

        std::pair<const Key&, const Value&> operator*() const {
            return {slot->key, slot->value};
        }
    };

    using iterator = Iterator;
    using const_iterator = ConstIterator;

private:
    std::vector<Slot> slots;
    size_t numOccupied{0};
    size_t numTombstones{0};
    size_t capacityMask{0};
    Hash hasher{};

    static size_t nextPowerOfTwo(size_t n) {
        size_t cap = 16;
        while (cap < n) cap <<= 1;
        return cap;
    }

    void rehash(size_t newCap) {
        std::vector<Slot> oldSlots = std::move(slots);
        slots.assign(newCap, Slot{});
        capacityMask = newCap - 1;
        numOccupied = 0;
        numTombstones = 0;

        for (auto& s : oldSlots) {
            if (s.state == State::Occupied) {
                insertInternal(s.key, std::move(s.value));
            }
        }
    }

    void insertInternal(const Key& key, Value val) {
        size_t idx = hasher(key) & capacityMask;
        size_t firstDeleted = capacityMask + 1;

        while (true) {
            if (slots[idx].state == State::Occupied) {
                if (slots[idx].key == key) {
                    slots[idx].value = std::move(val);
                    return;
                }
            } else if (slots[idx].state == State::Empty) {
                size_t targetIdx = (firstDeleted <= capacityMask) ? firstDeleted : idx;
                slots[targetIdx].key = key;
                slots[targetIdx].value = std::move(val);
                slots[targetIdx].state = State::Occupied;
                if (firstDeleted <= capacityMask) {
                    --numTombstones;
                }
                ++numOccupied;
                return;
            } else if (slots[idx].state == State::Deleted) {
                if (firstDeleted > capacityMask) {
                    firstDeleted = idx;
                }
            }
            idx = (idx + 1) & capacityMask;
        }
    }

public:
    explicit FlatOrderMap(size_t initialCap = 64) {
        size_t cap = nextPowerOfTwo(initialCap);
        slots.resize(cap);
        capacityMask = cap - 1;
    }

    size_t size() const noexcept { return numOccupied; }
    bool empty() const noexcept { return numOccupied == 0; }

    void clear() noexcept {
        for (auto& s : slots) {
            s.state = State::Empty;
        }
        numOccupied = 0;
        numTombstones = 0;
    }

    iterator end() {
        return iterator{slots.data() + slots.size(), slots.data() + slots.size()};
    }

    const_iterator end() const {
        return const_iterator{slots.data() + slots.size(), slots.data() + slots.size()};
    }

    iterator find(const Key& key) {
        if (slots.empty()) return end();
        size_t idx = hasher(key) & capacityMask;
        size_t startIdx = idx;

        while (slots[idx].state != State::Empty) {
            if (slots[idx].state == State::Occupied && slots[idx].key == key) {
                return iterator{&slots[idx], slots.data() + slots.size()};
            }
            idx = (idx + 1) & capacityMask;
            if (idx == startIdx) break;
        }
        return end();
    }

    const_iterator find(const Key& key) const {
        if (slots.empty()) return end();
        size_t idx = hasher(key) & capacityMask;
        size_t startIdx = idx;

        while (slots[idx].state != State::Empty) {
            if (slots[idx].state == State::Occupied && slots[idx].key == key) {
                return const_iterator{&slots[idx], slots.data() + slots.size()};
            }
            idx = (idx + 1) & capacityMask;
            if (idx == startIdx) break;
        }
        return end();
    }

    Value& operator[](const Key& key) {
        if ((numOccupied + numTombstones + 1) * 10 >= slots.size() * 7) {
            rehash(slots.size() * 2);
        }
        size_t idx = hasher(key) & capacityMask;
        size_t firstDeleted = capacityMask + 1;

        while (true) {
            if (slots[idx].state == State::Occupied) {
                if (slots[idx].key == key) {
                    return slots[idx].value;
                }
            } else if (slots[idx].state == State::Empty) {
                size_t targetIdx = (firstDeleted <= capacityMask) ? firstDeleted : idx;
                slots[targetIdx].key = key;
                slots[targetIdx].state = State::Occupied;
                if (firstDeleted <= capacityMask) {
                    --numTombstones;
                }
                ++numOccupied;
                return slots[targetIdx].value;
            } else if (slots[idx].state == State::Deleted) {
                if (firstDeleted > capacityMask) {
                    firstDeleted = idx;
                }
            }
            idx = (idx + 1) & capacityMask;
        }
    }

    bool erase(const Key& key) {
        if (slots.empty()) return false;
        size_t idx = hasher(key) & capacityMask;
        size_t startIdx = idx;

        while (slots[idx].state != State::Empty) {
            if (slots[idx].state == State::Occupied && slots[idx].key == key) {
                slots[idx].state = State::Deleted;
                --numOccupied;
                ++numTombstones;
                return true;
            }
            idx = (idx + 1) & capacityMask;
            if (idx == startIdx) break;
        }
        return false;
    }

    bool erase(iterator it) {
        if (it.slot && it.slot < slots.data() + slots.size() && it.slot->state == State::Occupied) {
            it.slot->state = State::Deleted;
            --numOccupied;
            ++numTombstones;
            return true;
        }
        return false;
    }

    bool erase(const_iterator it) {
        if (it.slot && it.slot < slots.data() + slots.size() && it.slot->state == State::Occupied) {
            const_cast<Slot*>(it.slot)->state = State::Deleted;
            --numOccupied;
            ++numTombstones;
            return true;
        }
        return false;
    }
};
