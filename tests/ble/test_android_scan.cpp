#include "android/AndroidScanState.h"
#include <iostream>
#include <stdexcept>
using namespace SimpleCommKit;
using namespace SimpleCommKit::Ble;
using namespace SimpleCommKit::Ble::AndroidDetail;
void check(bool ok) { if (!ok) throw std::runtime_error("Android scan invariant failed"); }
int main() {
    try {
        State s; std::vector<Event> events;
        s.sink = [&](Event e) { events.push_back(std::move(e)); };
        Peripheral p; p.value.address = "CC:A7:75:0D:58:F0";
        p.value.address_type = UNSPECIFIED; p.name = "im948-V3.05";
        p.value.manufacturer[0xccdd] = {0, 0xff, 0x80};
        s.advertisement(1, p); check(events.empty());
        s.scanning = true; s.epoch = 1; s.generation = 12;
        s.advertisement(1, p);
        check(events.size() == 1 && events.back().generation == 12);
        check(events.back().peripheral.value.manufacturer.at(0xccdd) == std::vector<uint8_t>({0, 0xff, 0x80}));
        p.name.clear(); p.value.rssi = -42; s.advertisement(1, p);
        check(s.devices.size() == 1 && events.back().peripheral.value.identifier == "im948-V3.05");
        p.name = "im948-V3.05"; p.value.address = "CC:A7:75:0D:58:F1";
        s.advertisement(1, p); check(s.devices.size() == 2);
        s.scanning = false; const auto count = events.size();
        s.advertisement(1, p); check(events.size() == count);
        s.scanning = true; s.epoch = 2; s.generation = 13; s.devices.clear();
        s.advertisement(1, p); check(events.size() == count && s.devices.empty());
        p.name.clear(); s.advertisement(2, p);
        check(events.back().peripheral.value.identifier.empty() && events.back().generation == 13);
        s.closing = true; s.advertisement(2, p); check(events.size() == count + 1);
        s.closing = false; s.linked = true; s.linkEpoch = 10; s.linkGeneration = 40;
        s.subscriptions[{"ae30", "ae02"}] = true;
        auto before = events.size();
        s.gattEvent(9, 1, "ae30", "ae02", {1}); check(events.size() == before);
        s.gattEvent(10, 1, "ae30", "ae02", {0, 255, 128});
        check(events.back().generation == 40 && events.back().data == std::vector<uint8_t>({0, 255, 128}));
        s.subscriptions.clear(); before = events.size();
        s.gattEvent(10, 1, "ae30", "ae02", {1}); check(events.size() == before);
        s.subscriptions[{"ae30", "ae02"}] = true;
        s.gattEvent(10, 2, "", "", {});
        check(!s.linked && s.subscriptions.empty() && events.back().type == EventType::Disconnected);
        before = events.size(); s.gattEvent(10, 0, "", "", {}); check(events.size() == before);
        std::cout << "Android scan identity, binary data, epoch and shutdown checks passed\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
