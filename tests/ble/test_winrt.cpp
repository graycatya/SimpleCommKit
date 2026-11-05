#include "windows/WinrtSupport.h"
#include <chrono>
#include <iostream>
#include <thread>
#include <mutex>
#include <algorithm>

using namespace SimpleCommKit;
using namespace SimpleCommKit::Ble;
using namespace SimpleCommKit::Ble::WindowsDetail;
using winrt::Windows::Devices::Bluetooth::BluetoothError;
using winrt::Windows::Devices::Bluetooth::BluetoothAddressType;
void check(bool value) { if (!value) throw std::runtime_error("Windows BLE assertion failed"); }
template<class F> void fails(ErrorCode expected, F action) {
    try { action(); } catch (const Failure& e) { check(e.code == expected); return; }
    throw std::runtime_error("Expected BLE failure");
}
// Independent synthetic handles exercise UUID selection without BLE hardware.
struct TestHandle {
    winrt::guid uuid{};
    int handle = 0;
    TestHandle(std::nullptr_t = nullptr) {}
    TestHandle(const wchar_t* uuid, int handle) : uuid(uuid), handle(handle) {}
    winrt::guid Uuid() const { return uuid; }
    explicit operator bool() const { return handle != 0; }
};
int main(int argc, char** argv) {
    try {
        check(bluetoothError(BluetoothError::ConsentRequired) == ErrorCodes::SimpleCommKitBlePermissionDenied);
        check(bluetoothError(BluetoothError::DisabledByPolicy) == ErrorCodes::SimpleCommKitBlePermissionDenied);
        check(bluetoothError(BluetoothError::DisabledByUser) == ErrorCodes::SimpleCommKitBlePermissionDenied);
        check(bluetoothError(BluetoothError::RadioNotAvailable) == ErrorCodes::SimpleCommKitBleBackendUnavailable);
        check(bluetoothError(BluetoothError::NotSupported) == ErrorCodes::SimpleCommKitBleNotSupported);
        check(bluetoothError(BluetoothError::Success) == ErrorCodes::SimpleCommKitBleSuccess);
        check(hresultError(E_ACCESSDENIED) == ErrorCodes::SimpleCommKitBlePermissionDenied);
        check(hresultError(HRESULT_FROM_WIN32(ERROR_TIMEOUT)) == ErrorCodes::SimpleCommKitBleTimeout);
        check(hresultError(HRESULT_FROM_WIN32(ERROR_CANCELLED)) == ErrorCodes::SimpleCommKitBleCancelled);
        check(hresultError(E_INVALIDARG) == ErrorCodes::SimpleCommKitBleInvalidArgument);
        auto pub = peripheral(0x001122aabbccULL, BluetoothAddressType::Public);
        auto random = peripheral(0x001122aabbccULL, BluetoothAddressType::Random);
        check(pub.value.address == "00:11:22:AA:BB:CC");
        check(pub.id != random.id && pub.value.identifier.empty());
        check(pub.value.address_type == PUBLIC && random.value.address_type == RANDOM);
        check(peripheral(1, BluetoothAddressType::Unspecified).value.address_type == UNSPECIFIED);
        using GattStatus = winrt::Windows::Devices::Bluetooth::GenericAttributeProfile::GattCommunicationStatus;
        checkGatt(GattStatus::Success);
        fails(ErrorCodes::SimpleCommKitBlePermissionDenied, [&] { checkGatt(GattStatus::AccessDenied); });
        fails(ErrorCodes::SimpleCommKitBleBackendUnavailable, [&] { checkGatt(GattStatus::Unreachable); });
        fails(ErrorCodes::SimpleCommKitBleGattError, [&] { checkGatt(GattStatus::ProtocolError); });
        check(uuidString(winrt::guid(L"0000180f-0000-1000-8000-00805f9b34fb")) == canonicalUuid("180f"));
        std::vector<TestHandle> handles{{L"0000180f-0000-1000-8000-00805f9b34fb", 7}};
        check(uniqueUuid(handles, "180F").handle == 7);
        fails(ErrorCodes::SimpleCommKitBleInvalidArgument, [&] { uniqueUuid(handles, "180a"); });
        fails(ErrorCodes::SimpleCommKitBleInvalidArgument, [&] { uniqueUuid(handles, "not-a-uuid"); });
        handles.emplace_back(L"0000180f-0000-1000-8000-00805f9b34fb", 9);
        fails(ErrorCodes::SimpleCommKitBleAmbiguousIdentity, [&] { uniqueUuid(handles, "180f"); });
        auto b = createBackend(); Operation op;
        fails(ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet, [&] { b->startScan(op); });
        fails(ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet, [&] { b->connect(op); });
        fails(ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet, [&] { b->connected(op); });
        fails(ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet, [&] { b->services(op); });
        fails(ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet, [&] { b->read("180f", "2a19", "", op); });
        Operation expired; expired.deadline = Clock::now();
        fails(ErrorCodes::SimpleCommKitBleTimeout, [&] { b->connect(expired); });
        Operation cancelled; cancelled.cancelled->store(true);
        fails(ErrorCodes::SimpleCommKitBleCancelled, [&] { b->scanning(cancelled); });
        if (argc > 1) {
            std::mutex mutex;
            int starts = 0, stops = 0, advertisements = 0, connects = 0, disconnects = 0, notifications = 0;
            ErrorCode asynchronousError = ErrorCodes::SimpleCommKitBleSuccess;
            b->setEventSink([&](Event e) {
                std::lock_guard<std::mutex> lock(mutex);
                if (e.type == EventType::ScanStarted) ++starts;
                if (e.type == EventType::ScanStopped) ++stops;
                if (e.type == EventType::Advertisement) {
                    check(e.peripheral.value.identifier == e.peripheral.name);
                    ++advertisements;
                }
                if (e.type == EventType::Connected) ++connects;
                if (e.type == EventType::Disconnected) ++disconnects;
                if (e.type == EventType::Notification) ++notifications;
                if (e.type == EventType::Error) asynchronousError = e.error;
            });
            struct ProbeCleanup {
                IBleCentralBackend& backend;
                ~ProbeCleanup() { backend.setEventSink({}); backend.shutdown(); }
            } cleanup{*b};
            auto adapters = b->adapters(op);
            std::cout << "LE adapters: " << adapters.size() << ", enabled: " << b->enabled(op) << '\n';
            bool selected = false;
            for (auto& a : adapters) {
                try { b->selectAdapter(a.id, op); selected = true; break; }
                catch (const Failure& e) { if (e.code != ErrorCodes::SimpleCommKitBleNotSupported) throw; }
            }
            if (selected) {
                const bool connectionProbe = argc > 2 && (std::string(argv[1]) == "--connect" || std::string(argv[1]) == "--read");
                const int scans = connectionProbe ? 1 : 3;
                for (int i = 0; i < scans; ++i) {
                    b->startScan(op);
                    std::this_thread::sleep_for(std::chrono::seconds(connectionProbe ? 15 : 2));
                    std::cout << "scan " << i + 1 << ": " << b->peripherals(DeviceFilter::All, op).size() << " devices\n";
                    b->stopScan(op); b->stopScan(op); check(!b->scanning(op));
                    int count;
                    { std::lock_guard<std::mutex> lock(mutex); count = advertisements;
                      check(starts == i + 1 && stops == i + 1);
                      if (asynchronousError != ErrorCodes::SimpleCommKitBleSuccess) throw Failure(asynchronousError); }
                    std::this_thread::sleep_for(std::chrono::milliseconds(100));
                    { std::lock_guard<std::mutex> lock(mutex); check(advertisements == count); }
                }
            }
            if (argc > 2 && (std::string(argv[1]) == "--connect" || std::string(argv[1]) == "--read")) {
                auto all = b->peripherals(DeviceFilter::All, Operation{});
                auto target = std::find_if(all.begin(), all.end(), [&](const auto& p) { return p.value.address == argv[2]; });
                if (target == all.end()) throw std::runtime_error("Requested peripheral was not seen in the final scan");
                b->selectPeripheral(target->id, Operation{});
                for (int cycle = 0; cycle < 2; ++cycle) {
                    Operation connection; connection.generation = static_cast<uint64_t>(cycle * 2 + 1);
                    b->connect(connection); check(b->connected(connection));
                    std::cout << "Connected: MTU " << b->mtu(connection) << ", paired " << b->paired(connection) << '\n';
                    Operation gatt; gatt.generation = connection.generation;
                    auto tree = b->services(gatt);
                    std::cout << "GATT services: " << tree.size() << '\n';
                    const bool targetedRead = argc > 4 && std::string(argv[1]) == "--read";
                    if (std::string(argv[1]) == "--read" && !targetedRead)
                        throw std::runtime_error("Usage: --read <MAC> <service UUID> <characteristic UUID>");
                    if (targetedRead) {
                        auto value = b->read(argv[3], argv[4], "", gatt);
                        std::cout << "Targeted read " << argv[3] << '/' << argv[4] << ": " << value.size() << " bytes, connected=" << b->connected(gatt) << std::endl;
                    } else for (auto& service : tree) {
                        std::cout << "  service " << service.uuid << " characteristics " << service.characteristics.size() << '\n';
                        for (auto& c : service.characteristics) {
                            std::cout << "    characteristic " << c.uuid << " read=" << c.can_read << std::endl;
                            // Read only standard device-information/battery attributes.
                            if (c.can_read && (service.uuid == canonicalUuid("1800") || service.uuid == canonicalUuid("1801") || service.uuid == canonicalUuid("180a") || service.uuid == canonicalUuid("180f"))) {
                                auto value = b->read(service.uuid, c.uuid, "", gatt);
                                std::cout << "    read " << c.uuid << ": " << value.size() << " bytes";
                                for (auto byte : value) std::cout << ' ' << std::hex << static_cast<int>(byte);
                                std::cout << std::dec << std::endl;
                            }
                            for (auto& descriptor : c.descriptors_uuid) if (descriptor == canonicalUuid("2902")) {
                                auto value = b->read(service.uuid, c.uuid, descriptor, gatt);
                                std::cout << "    read CCCD: " << value.size() << " bytes\n";
                            }
                            if (c.can_notify || c.can_indicate) {
                                b->subscribe(service.uuid, c.uuid, c.can_notify ? SubscriptionType::Notify : SubscriptionType::Indicate, gatt);
                                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                                b->unsubscribe(service.uuid, c.uuid, gatt);
                                int count;
                                { std::lock_guard<std::mutex> lock(mutex); count = notifications; }
                                std::this_thread::sleep_for(std::chrono::milliseconds(100));
                                { std::lock_guard<std::mutex> lock(mutex); check(count == notifications); }
                                std::cout << (c.can_notify ? "    notify" : "    indicate") << "/unsubscribe passed: " << c.uuid << '\n';
                            }
                        }
                    }
                    Operation disconnection; disconnection.generation = connection.generation + 1;
                    b->disconnect(disconnection); b->disconnect(disconnection); check(!b->connected(disconnection));
                    { std::lock_guard<std::mutex> lock(mutex); check(connects == cycle + 1 && disconnects == cycle + 1); }
                }
                { std::lock_guard<std::mutex> lock(mutex);
                  std::cout << "Connection/reconnection probe passed, notification payloads: " << notifications << '\n'; }
            }
            b->setEventSink({});
            b->shutdown(); // Drain callbacks before the probe captures leave scope.
        }
        b->shutdown(); b->shutdown();
        std::cout << "Windows BLE checks passed\n";
    } catch (const Failure& e) { std::cerr << "BLE failure: " << e.code << " " << e.what() << '\n'; return 1; }
      catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
