#include "WinrtSupport.h"
#include <winrt/Windows.Devices.Bluetooth.Advertisement.h>
#include <winrt/Windows.Devices.Enumeration.h>
#include <winrt/Windows.Devices.Radios.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Storage.Streams.h>
#include <mutex>
#include <thread>

namespace SimpleCommKit::Ble {
namespace {
using namespace winrt;
using namespace winrt::Windows::Foundation;
using namespace winrt::Windows::Devices::Bluetooth;
using namespace winrt::Windows::Devices::Bluetooth::Advertisement;
using namespace winrt::Windows::Devices::Enumeration;
using namespace winrt::Windows::Devices::Radios;
using namespace WindowsDetail;
using namespace winrt::Windows::Devices::Bluetooth::GenericAttributeProfile;
using namespace winrt::Windows::Storage::Streams;

// Each calling executor owns its MTA initialization. No apartment is changed on
// an existing STA (the static enabled query may be called from a UI thread).
struct Apartment {
    bool owned = false;
    Apartment() {
        HRESULT hr = RoInitialize(RO_INIT_MULTITHREADED);
        if (SUCCEEDED(hr)) owned = true;
        else if (hr != RPC_E_CHANGED_MODE) check_hresult(hr);
    }
    ~Apartment() { if (owned) RoUninitialize(); }
};
template<class F> auto native(F work) -> decltype(work()) {
    try { thread_local Apartment apartment; return work(); }
    catch (const hresult_error& e) { throw Failure(hresultError(e.code()), to_string(e.message())); }
}
template<class T> auto await(T async, const Operation& op) {
    try {
        while (async.Status() == AsyncStatus::Started) {
            op.check();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
        op.check();
        return async.GetResults();
    } catch (...) { try { async.Cancel(); } catch (...) {} throw; }
}
[[noreturn]] void unsupported(const Operation& op) {
    op.check(); throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
}
struct State {
    std::mutex mutex;
    EventSink sink;
    bool closing = false, active = false;
    uint64_t epoch = 0, connectionEpoch = 0, gattRevision = 0, subscriptionRevision = 0;
    bool linkActive = false;
    std::map<std::pair<std::string, std::string>, uint64_t> subscriptions;
    std::map<std::string, Peripheral> discovered;
    void emit(Event e) { if (!closing && sink) sink(std::move(e)); }
};
class WinrtBackend final : public IBleCentralBackend {
    std::shared_ptr<State> state = std::make_shared<State>();
    BluetoothAdapter adapter{nullptr};
    Radio radio{nullptr};
    BluetoothLEAdvertisementWatcher watcher{nullptr};
    event_token received{}, stopped{}, radioChanged{};
    std::string selected;
    Peripheral chosen;
    BluetoothLEDevice device{nullptr};
    GattSession session{nullptr};
    event_token connectionChanged{}, servicesChanged{};
    std::vector<GattDeviceService> serviceHandles;
    uint64_t serviceRevision = 0;
    struct Subscription {
        GattCharacteristic characteristic{nullptr};
        event_token token{};
    };
    std::map<std::pair<std::string, std::string>, Subscription> subscriptions;
    void requireSelected(const Operation& op) {
        op.check();
        if (selected.empty()) throw Failure(ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
    }
    void requireConnected(const Operation& op) {
        requireSelected(op);
        std::lock_guard<std::mutex> lock(state->mutex);
        if (!device || !state->linkActive || device.ConnectionStatus() != BluetoothConnectionStatus::Connected)
            throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable, "Peripheral is disconnected");
    }
    BluetoothLEDevice openDevice(const Operation& op) {
        requireSelected(op);
        std::string hex;
        for (char c : chosen.value.address) if (c != ':') hex += c;
        auto type = chosen.value.address_type == PUBLIC ? BluetoothAddressType::Public :
            chosen.value.address_type == RANDOM ? BluetoothAddressType::Random : BluetoothAddressType::Unspecified;
        auto value = await(BluetoothLEDevice::FromBluetoothAddressAsync(std::stoull(hex, nullptr, 16), type), op);
        if (!value) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable, "Windows could not resolve the peripheral");
        return value;
    }
    void clearSubscriptions() noexcept {
        { std::lock_guard<std::mutex> lock(state->mutex); state->subscriptions.clear(); }
        for (auto& item : subscriptions) {
            try { item.second.characteristic.ValueChanged(item.second.token); } catch (...) {}
        }
        subscriptions.clear();
    }
    void closeServices() noexcept {
        for (auto& value : serviceHandles) { try { value.Close(); } catch (...) {} }
        serviceHandles.clear();
    }
    void releaseConnection(bool notify = false, uint64_t generation = 0) noexcept {
        { std::lock_guard<std::mutex> lock(state->mutex);
          const bool wasConnected = state->linkActive;
          state->linkActive = false; ++state->connectionEpoch; ++state->gattRevision;
          if (notify && wasConnected) state->emit(Event(EventType::Disconnected, generation)); }
        clearSubscriptions(); closeServices();
        if (device) {
            try { device.ConnectionStatusChanged(connectionChanged); device.GattServicesChanged(servicesChanged); } catch (...) {}
        }
        if (session) {
            try { session.MaintainConnection(false); session.Close(); } catch (...) {}
            session = nullptr;
        }
        if (device) { try { device.Close(); } catch (...) {} device = nullptr; }
    }
    void refreshServices(const Operation& op) {
        for (;;) {
            requireConnected(op);
            uint64_t revision;
            { std::lock_guard<std::mutex> lock(state->mutex); revision = state->gattRevision; }
            if (revision == serviceRevision && !serviceHandles.empty()) return;
            clearSubscriptions(); closeServices();
            auto result = await(device.GetGattServicesAsync(BluetoothCacheMode::Uncached), op);
            checkGatt(result.Status()); requireConnected(op);
            for (auto value : result.Services()) serviceHandles.push_back(value);
            { std::lock_guard<std::mutex> lock(state->mutex);
              if (revision == state->gattRevision) { serviceRevision = revision; return; } }
            // Services changed during discovery. Drop the stale handles and retry
            // under the same operation deadline rather than publishing stale GATT.
        }
    }
    GattCharacteristic characteristic(const std::string& service, const std::string& uuid, const Operation& op) {
        refreshServices(op);
        auto parent = uniqueUuid(serviceHandles, service);
        auto result = await(parent.GetCharacteristicsAsync(BluetoothCacheMode::Uncached), op);
        checkGatt(result.Status()); requireConnected(op);
        std::vector<GattCharacteristic> values;
        for (auto value : result.Characteristics()) values.push_back(value);
        return uniqueUuid(values, uuid);
    }
    GattDescriptor descriptor(const GattCharacteristic& c, const std::string& uuid, const Operation& op) {
        auto result = await(c.GetDescriptorsAsync(BluetoothCacheMode::Uncached), op);
        checkGatt(result.Status()); requireConnected(op);
        std::vector<GattDescriptor> values;
        for (auto value : result.Descriptors()) values.push_back(value);
        return uniqueUuid(values, uuid);
    }
    static std::vector<uint8_t> bytes(const IBuffer& buffer) {
        auto reader = DataReader::FromBuffer(buffer);
        std::vector<uint8_t> result(reader.UnconsumedBufferLength()); reader.ReadBytes(result); return result;
    }
    static IBuffer buffer(const std::vector<uint8_t>& value) {
        DataWriter writer; writer.WriteBytes(value); return writer.DetachBuffer();
    }
    static bool has(GattCharacteristicProperties value, GattCharacteristicProperties flag) {
        return (value & flag) != GattCharacteristicProperties::None;
    }
    void requireAdapter(const Operation& op) {
        op.check(); if (!adapter) throw Failure(ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
    }
    void detachScan() noexcept {
        if (!watcher) return;
        try { watcher.Received(received); watcher.Stopped(stopped); watcher.Stop(); } catch (...) {}
        watcher = nullptr;
    }
public:
    ~WinrtBackend() override { shutdown(); }
    Capabilities capabilities() const override {
        Capabilities c; c.paired_devices = true; c.pairing_state = true; c.unpair = true; c.mtu = true; return c;
    }
    void setEventSink(EventSink sink) override {
        std::lock_guard<std::mutex> lock(state->mutex); state->sink = std::move(sink);
    }
    void shutdown() noexcept override {
        { std::lock_guard<std::mutex> lock(state->mutex);
          state->closing = true; state->active = false; ++state->epoch; state->sink = {}; }
        detachScan(); releaseConnection();
        if (radio) { try { radio.StateChanged(radioChanged); } catch (...) {} }
        radio = nullptr; adapter = nullptr;
    }
    bool enabled(const Operation& op) override {
        return native([&] {
            auto a = await(BluetoothAdapter::GetDefaultAsync(), op);
            if (!a || !a.IsLowEnergySupported()) return false;
            auto r = await(a.GetRadioAsync(), op);
            if (!r) throw Failure(ErrorCodes::SimpleCommKitBlePermissionDenied);
            return r.State() == RadioState::On;
        });
    }
    std::vector<Adapter> adapters(const Operation& op) override {
        return native([&] {
            std::vector<Adapter> out;
            auto list = await(DeviceInformation::FindAllAsync(BluetoothAdapter::GetDeviceSelector()), op);
            for (auto info : list) {
                auto a = await(BluetoothAdapter::FromIdAsync(info.Id()), op);
                if (!a || !a.IsLowEnergySupported()) continue;
                Adapter value; value.id = to_string(a.DeviceId());
                value.value.dev_identifier = value.id;
                value.value.dev_address = address(a.BluetoothAddress());
                out.push_back(std::move(value));
            }
            return out;
        });
    }
    void selectAdapter(const std::string& id, const Operation& op) override {
        native([&] {
            auto a = await(BluetoothAdapter::GetDefaultAsync(), op);
            if (!a) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
            // AdvertisementWatcher has no adapter selection API.
            if (to_string(a.DeviceId()) != id) throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
            if (!a.IsLowEnergySupported()) throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
            auto r = await(a.GetRadioAsync(), op);
            if (!r) throw Failure(ErrorCodes::SimpleCommKitBlePermissionDenied);
            stopScan(op); releaseConnection(true, op.generation);
            if (radio) radio.StateChanged(radioChanged);
            adapter = a; radio = r; selected.clear();
            std::weak_ptr<State> weak = state;
            radioChanged = radio.StateChanged([weak](const Radio& sender, const winrt::Windows::Foundation::IInspectable&) noexcept {
                try { if (auto s = weak.lock()) {
                    const bool on = sender.State() == RadioState::On;
                    std::lock_guard<std::mutex> lock(s->mutex);
                    s->emit(Event(on ? EventType::PowerOn : EventType::PowerOff));
                }} catch (...) {}
            });
            std::lock_guard<std::mutex> lock(state->mutex); state->discovered.clear();
        });
    }
    bool powered(const Operation& op) override {
        return native([&] { requireAdapter(op); return radio.State() == RadioState::On; });
    }
    void setPowered(bool, const Operation& op) override { unsupported(op); }
    void startScan(const Operation& op) override {
        native([&] {
            requireAdapter(op);
            { std::lock_guard<std::mutex> lock(state->mutex); if (state->active) return; }
            if (radio.State() != RadioState::On) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable, "Bluetooth radio is off");
            detachScan();
            watcher = BluetoothLEAdvertisementWatcher(); watcher.ScanningMode(BluetoothLEScanningMode::Active);
            uint64_t epoch;
            { std::lock_guard<std::mutex> lock(state->mutex); epoch = ++state->epoch; state->discovered.clear(); }
            std::weak_ptr<State> weak = state;
            received = watcher.Received([weak, epoch, generation = op.generation](auto const&, const BluetoothLEAdvertisementReceivedEventArgs& args) noexcept {
                try {
                    auto p = peripheral(args.BluetoothAddress(), args.BluetoothAddressType());
                    if (!args.BluetoothAddress()) return; // Anonymous advertisements cannot be selected.
                    p.name = to_string(args.Advertisement().LocalName());
                    p.value.rssi = args.RawSignalStrengthInDBm();
                    for (auto data : args.Advertisement().ManufacturerData()) {
                        auto reader = winrt::Windows::Storage::Streams::DataReader::FromBuffer(data.Data());
                        std::vector<uint8_t> bytes(reader.UnconsumedBufferLength());
                        reader.ReadBytes(bytes); p.value.manufacturer[data.CompanyId()] = std::move(bytes);
                    }
                    if (auto s = weak.lock()) {
                        std::lock_guard<std::mutex> lock(s->mutex);
                        if (s->closing || !s->active || s->epoch != epoch) return;
                        auto old = s->discovered.find(p.id);
                        if (old != s->discovered.end()) {
                            if (p.name.empty()) p.name = old->second.name;
                            for (auto& item : old->second.value.manufacturer) p.value.manufacturer.emplace(item);
                        }
                        p.value.identifier = p.name;
                        s->discovered[p.id] = p;
                        Event e(EventType::Advertisement, generation); e.peripheral = std::move(p); s->emit(std::move(e));
                    }
                } catch (...) {
                    if (auto s = weak.lock()) { std::lock_guard<std::mutex> lock(s->mutex);
                        if (s->active && s->epoch == epoch) { Event e(EventType::Error); e.error = ErrorCodes::SimpleCommKitBleBackendUnavailable; s->emit(e); } }
                }
            });
            stopped = watcher.Stopped([weak, epoch](auto const&, const BluetoothLEAdvertisementWatcherStoppedEventArgs& args) noexcept {
                try { if (auto s = weak.lock()) {
                    auto error = bluetoothError(args.Error());
                    std::lock_guard<std::mutex> lock(s->mutex);
                    if (s->closing || !s->active || s->epoch != epoch) return;
                    s->active = false;
                    s->emit(Event(EventType::ScanStopped));
                    if (error != ErrorCodes::SimpleCommKitBleSuccess) { Event e(EventType::Error); e.error = error; s->emit(e); }
                }} catch (...) {}
            });
            try {
                std::lock_guard<std::mutex> lock(state->mutex);
                op.check(); watcher.Start();
                state->active = true; state->emit(Event(EventType::ScanStarted));
            } catch (...) { detachScan(); throw; }
        });
    }
    void stopScan(const Operation& op) override {
        native([&] {
            op.check();
            { std::lock_guard<std::mutex> lock(state->mutex);
              ++state->epoch;
              if (state->active) { state->active = false; state->emit(Event(EventType::ScanStopped)); } }
            detachScan();
        });
    }
    bool scanning(const Operation& op) override {
        op.check(); std::lock_guard<std::mutex> lock(state->mutex); return state->active;
    }
    std::vector<Peripheral> peripherals(DeviceFilter filter, const Operation& op) override {
        return native([&] {
            requireAdapter(op);
            std::vector<Peripheral> out;
            if (filter == DeviceFilter::All) {
                std::lock_guard<std::mutex> lock(state->mutex);
                for (auto& item : state->discovered) out.push_back(item.second);
                return out;
            }
            auto selector = filter == DeviceFilter::Paired ? BluetoothLEDevice::GetDeviceSelectorFromPairingState(true) :
                BluetoothLEDevice::GetDeviceSelectorFromConnectionStatus(BluetoothConnectionStatus::Connected);
            auto list = await(DeviceInformation::FindAllAsync(selector), op);
            for (auto info : list) {
                const bool current = device && device.DeviceId() == info.Id();
                auto value = current ? device : await(BluetoothLEDevice::FromIdAsync(info.Id()), op);
                if (!value) continue;
                try {
                    auto p = peripheral(value.BluetoothAddress(), value.BluetoothAddressType());
                    p.name = to_string(value.Name()); p.value.identifier = p.name;
                    if (!current) value.Close();
                    out.push_back(p);
                    std::lock_guard<std::mutex> lock(state->mutex);
                    auto old = state->discovered.find(p.id);
                    if (old == state->discovered.end()) state->discovered[p.id] = p;
                } catch (...) { if (!current) { try { value.Close(); } catch (...) {} } throw; }
            }
            return out;
        });
    }
    void selectPeripheral(const std::string& id, const Operation& op) override {
        native([&] {
            requireAdapter(op);
            Peripheral value;
            { std::lock_guard<std::mutex> lock(state->mutex);
              if (!state->discovered.count(id)) throw Failure(ErrorCodes::SimpleCommKitBleCurrentPeripheralSetError);
              value = state->discovered.at(id); }
            releaseConnection(true, op.generation); selected = id; chosen = std::move(value);
        });
    }
    void connect(const Operation& op) override {
        native([&] {
            requireSelected(op); releaseConnection();
            try {
                device = openDevice(op);
                auto access = DeviceAccessInformation::CreateFromId(device.DeviceId()).CurrentStatus();
                if (access == DeviceAccessStatus::DeniedByUser || access == DeviceAccessStatus::DeniedBySystem)
                    throw Failure(ErrorCodes::SimpleCommKitBlePermissionDenied);
                session = await(GattSession::FromDeviceIdAsync(device.BluetoothDeviceId()), op);
                if (!session) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
                if (session.CanMaintainConnection()) session.MaintainConnection(true);
                uint64_t epoch;
                { std::lock_guard<std::mutex> lock(state->mutex); epoch = state->connectionEpoch; }
                std::weak_ptr<State> weak = state;
                connectionChanged = device.ConnectionStatusChanged([weak, epoch, session = session, generation = op.generation](const BluetoothLEDevice& sender, auto const&) noexcept {
                    try { if (auto s = weak.lock()) {
                        const bool on = sender.ConnectionStatus() == BluetoothConnectionStatus::Connected;
                        std::lock_guard<std::mutex> lock(s->mutex);
                        if (s->closing || s->connectionEpoch != epoch || !s->linkActive || on) return;
                        s->linkActive = false; ++s->gattRevision; s->subscriptions.clear();
                        s->emit(Event(EventType::Disconnected, generation));
                        if (session.CanMaintainConnection()) session.MaintainConnection(false);
                    }} catch (...) {}
                });
                servicesChanged = device.GattServicesChanged([weak, epoch](auto const&, auto const&) noexcept {
                    try { if (auto s = weak.lock()) {
                        std::lock_guard<std::mutex> lock(s->mutex);
                        if (s->connectionEpoch == epoch) {
                            ++s->gattRevision;
                            if (!s->subscriptions.empty()) {
                                s->subscriptions.clear();
                                Event error(EventType::Error); error.error = ErrorCodes::SimpleCommKitBleGattError; s->emit(error);
                            }
                        }
                    }} catch (...) {}
                });
                uint64_t discoveredRevision;
                { std::lock_guard<std::mutex> lock(state->mutex); discoveredRevision = state->gattRevision; }
                auto result = await(device.GetGattServicesAsync(BluetoothCacheMode::Uncached), op);
                checkGatt(result.Status());
                while (device.ConnectionStatus() != BluetoothConnectionStatus::Connected) {
                    op.check(); std::this_thread::sleep_for(std::chrono::milliseconds(5));
                }
                for (auto value : result.Services()) serviceHandles.push_back(value);
                std::lock_guard<std::mutex> lock(state->mutex);
                op.check();
                if (device.ConnectionStatus() != BluetoothConnectionStatus::Connected)
                    throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable, "Disconnected during service discovery");
                state->linkActive = true; serviceRevision = discoveredRevision;
                state->emit(Event(EventType::Connected, op.generation));
            } catch (...) { releaseConnection(); throw; }
        });
    }
    void disconnect(const Operation& op) override {
        native([&] { requireSelected(op); releaseConnection(true, op.generation); });
    }
    bool connected(const Operation& op) override {
        return native([&] {
            requireSelected(op);
            std::lock_guard<std::mutex> lock(state->mutex);
            return device && state->linkActive && device.ConnectionStatus() == BluetoothConnectionStatus::Connected;
        });
    }
    bool connectable(const Operation& op) override { unsupported(op); }
    bool paired(const Operation& op) override {
        return native([&] {
            requireSelected(op); auto value = device ? device : openDevice(op);
            const bool result = value.DeviceInformation().Pairing().IsPaired();
            if (!device) value.Close(); return result;
        });
    }
    void unpair(const Operation& op) override {
        native([&] {
            requireSelected(op);
            auto value = device ? device : openDevice(op);
            auto pairing = value.DeviceInformation().Pairing();
            if (!device) value.Close();
            releaseConnection(true, op.generation);
            auto result = await(pairing.UnpairAsync(), op);
            switch (result.Status()) {
            case DeviceUnpairingResultStatus::Unpaired: case DeviceUnpairingResultStatus::AlreadyUnpaired: return;
            case DeviceUnpairingResultStatus::AccessDenied: throw Failure(ErrorCodes::SimpleCommKitBlePermissionDenied);
            default: throw Failure(ErrorCodes::SimpleCommKitBleGattError, "Windows unpair failed");
            }
        });
    }
    int16_t txPower(const Operation& op) override { unsupported(op); }
    uint16_t mtu(const Operation& op) override {
        return native([&] { requireConnected(op); return session.MaxPduSize(); });
    }
    std::vector<SimpleCommKitBleService> services(const Operation& op) override {
        return native([&] {
            refreshServices(op); std::vector<SimpleCommKitBleService> out;
            uint64_t revision;
            { std::lock_guard<std::mutex> lock(state->mutex); revision = state->gattRevision; }
            for (auto parent : serviceHandles) {
                SimpleCommKitBleService service; service.uuid = uuidString(parent.Uuid());
                auto result = await(parent.GetCharacteristicsAsync(BluetoothCacheMode::Uncached), op);
                checkGatt(result.Status());
                for (auto c : result.Characteristics()) {
                    SimpleCommKitBleCharacteristic value{}; value.uuid = uuidString(c.Uuid());
                    const auto flags = c.CharacteristicProperties();
                    value.can_read = has(flags, GattCharacteristicProperties::Read);
                    value.can_write_request = has(flags, GattCharacteristicProperties::Write);
                    value.can_write_command = has(flags, GattCharacteristicProperties::WriteWithoutResponse);
                    value.can_notify = has(flags, GattCharacteristicProperties::Notify);
                    value.can_indicate = has(flags, GattCharacteristicProperties::Indicate);
                    if (value.can_read) value.capabilities.push_back("read");
                    if (value.can_write_request) value.capabilities.push_back("write_request");
                    if (value.can_write_command) value.capabilities.push_back("write_command");
                    if (value.can_notify) value.capabilities.push_back("notify");
                    if (value.can_indicate) value.capabilities.push_back("indicate");
                    auto descriptors = await(c.GetDescriptorsAsync(BluetoothCacheMode::Uncached), op);
                    checkGatt(descriptors.Status());
                    for (auto d : descriptors.Descriptors()) value.descriptors_uuid.push_back(uuidString(d.Uuid()));
                    service.characteristics.push_back(std::move(value));
                }
                out.push_back(std::move(service));
            }
            requireConnected(op);
            { std::lock_guard<std::mutex> lock(state->mutex);
              if (revision != state->gattRevision) throw Failure(ErrorCodes::SimpleCommKitBleGattError, "GATT changed during discovery; retry"); }
            return out;
        });
    }
    std::vector<uint8_t> read(const std::string& s, const std::string& c, const std::string& d, const Operation& op) override {
        return native([&] {
            auto ch = characteristic(s, c, op);
            if (d.empty() && !has(ch.CharacteristicProperties(), GattCharacteristicProperties::Read)) unsupported(op);
            auto result = d.empty() ? await(ch.ReadValueAsync(BluetoothCacheMode::Uncached), op) :
                await(descriptor(ch, d, op).ReadValueAsync(BluetoothCacheMode::Uncached), op);
            checkGatt(result.Status()); requireConnected(op); return bytes(result.Value());
        });
    }
    void write(const std::string& s, const std::string& c, const std::string& d, const std::vector<uint8_t>& value, WriteType type, const Operation& op) override {
        native([&] {
            auto ch = characteristic(s, c, op); auto payload = buffer(value);
            if (!d.empty()) checkGatt(await(descriptor(ch, d, op).WriteValueAsync(payload), op));
            else {
                const bool request = type == WriteType::Request;
                if (!has(ch.CharacteristicProperties(), request ? GattCharacteristicProperties::Write : GattCharacteristicProperties::WriteWithoutResponse)) unsupported(op);
                checkGatt(await(ch.WriteValueAsync(payload, request ? GattWriteOption::WriteWithResponse : GattWriteOption::WriteWithoutResponse), op));
            }
            requireConnected(op);
        });
    }
    void subscribe(const std::string& s, const std::string& c, SubscriptionType type, const Operation& op) override {
        native([&] {
            const auto key = std::make_pair(canonicalUuid(s), canonicalUuid(c));
            auto ch = characteristic(s, c, op);
            const bool notify = type == SubscriptionType::Notify;
            if (!has(ch.CharacteristicProperties(), notify ? GattCharacteristicProperties::Notify : GattCharacteristicProperties::Indicate)) unsupported(op);
            if (subscriptions.count(key)) unsubscribe(s, c, op);
            uint64_t epoch, revision, gattRevision;
            { std::lock_guard<std::mutex> lock(state->mutex);
              epoch = state->connectionEpoch; revision = ++state->subscriptionRevision; gattRevision = state->gattRevision; }
            std::weak_ptr<State> weak = state;
            auto token = ch.ValueChanged([weak, key, epoch, revision, generation = op.generation](auto const&, const GattValueChangedEventArgs& args) noexcept {
                try {
                    auto payload = bytes(args.CharacteristicValue());
                    if (auto state = weak.lock()) {
                        std::lock_guard<std::mutex> lock(state->mutex);
                        auto it = state->subscriptions.find(key);
                        if (state->closing || !state->linkActive || state->connectionEpoch != epoch || it == state->subscriptions.end() || it->second != revision) return;
                        Event e(EventType::Notification, generation); e.service = key.first; e.characteristic = key.second;
                        e.data = std::move(payload); state->emit(std::move(e));
                    }
                } catch (...) {}
            });
            try {
                checkGatt(await(ch.WriteClientCharacteristicConfigurationDescriptorAsync(notify ?
                    GattClientCharacteristicConfigurationDescriptorValue::Notify : GattClientCharacteristicConfigurationDescriptorValue::Indicate), op));
                requireConnected(op);
                std::lock_guard<std::mutex> lock(state->mutex);
                if (epoch != state->connectionEpoch || gattRevision != state->gattRevision || !state->linkActive)
                    throw Failure(ErrorCodes::SimpleCommKitBleGattError, "Connection or GATT changed during subscription");
                subscriptions.emplace(key, Subscription{ch, token}); state->subscriptions[key] = revision;
            } catch (...) {
                try { ch.ValueChanged(token); } catch (...) {}
                throw;
            }
        });
    }
    void unsubscribe(const std::string& s, const std::string& c, const Operation& op) override {
        native([&] {
            op.check(); const auto key = std::make_pair(canonicalUuid(s), canonicalUuid(c));
            { std::lock_guard<std::mutex> lock(state->mutex); state->subscriptions.erase(key); }
            auto it = subscriptions.find(key); if (it == subscriptions.end()) return;
            auto sub = it->second; subscriptions.erase(it); sub.characteristic.ValueChanged(sub.token);
            checkGatt(await(sub.characteristic.WriteClientCharacteristicConfigurationDescriptorAsync(
                GattClientCharacteristicConfigurationDescriptorValue::None), op));
        });
    }
};
}
std::unique_ptr<IBleCentralBackend> createBackend() { return std::make_unique<WinrtBackend>(); }
}
