#pragma once
#include "../BleBackend.h"
#include <map>
#include <mutex>

namespace SimpleCommKit::Ble::AndroidDetail {
struct State {
    std::mutex mutex;
    EventSink sink;
    bool closing = false, scanning = false;
    uint64_t epoch = 0, generation = 0;
    std::map<std::string, Peripheral> devices;
    bool linked = false;
    uint64_t linkEpoch = 0, linkGeneration = 0;
    std::shared_ptr<std::atomic<bool>> pendingCancelled;
    std::map<std::pair<std::string, std::string>, bool> subscriptions;
    // Sink only queues events on the facade; application callbacks run later.
    void emit(Event event) { if (!closing && sink) sink(std::move(event)); }
    void advertisement(uint64_t scanEpoch, Peripheral p) {
        std::lock_guard<std::mutex> lock(mutex);
        if (closing || !scanning || epoch != scanEpoch || p.value.address.empty()) return;
        p.id = "android:" + p.value.address;
        auto old = devices.find(p.id);
        if (p.name.empty() && old != devices.end()) p.name = old->second.name;
        p.value.identifier = p.name;
        devices[p.id] = p;
        Event event(EventType::Advertisement, generation); event.peripheral = std::move(p);
        emit(std::move(event));
    }
    void gattEvent(uint64_t token, int type, const std::string& service,
                   const std::string& characteristic, std::vector<uint8_t> data) {
        std::lock_guard<std::mutex> lock(mutex);
        if (closing || linkEpoch != token) return;
        if (type == 0 || type == 2) {
            const bool wasLinked = linked;
            linked = false; ++linkEpoch; subscriptions.clear();
            if (type == 2) { Event e(EventType::Error, linkGeneration); e.error = ErrorCodes::SimpleCommKitBleGattError; emit(e); }
            if (wasLinked) emit(Event(EventType::Disconnected, linkGeneration));
        } else if (type == 1 && linked && subscriptions.count({service, characteristic})) {
            Event e(EventType::Notification, linkGeneration);
            e.service = service; e.characteristic = characteristic; e.data = std::move(data); emit(std::move(e));
        }
    }
};
}
