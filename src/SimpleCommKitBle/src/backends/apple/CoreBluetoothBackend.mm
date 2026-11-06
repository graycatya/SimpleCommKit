// CoreBluetooth objects are confined to a private serial dispatch queue.
// Delegate callbacks hold weak state; the public runtime owns user callbacks.
#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>
#include "../BleRuntime.h"
#include <set>

namespace SimpleCommKit::Ble::Apple {
struct State;
std::string string(NSString* value) { return value ? value.UTF8String : ""; }
std::vector<uint8_t> bytes(NSData* value) {
    if (!value.length) return {};
    const auto* first = static_cast<const uint8_t*>(value.bytes);
    return {first, first + value.length};
}
ErrorCode errorCode(NSError* error) {
    if (!error) return ErrorCodes::SimpleCommKitBleSuccess;
    if ([error.domain isEqualToString:CBATTErrorDomain]) {
        if (error.code == CBATTErrorInsufficientAuthentication ||
            error.code == CBATTErrorInsufficientAuthorization ||
            error.code == CBATTErrorInsufficientEncryption ||
            error.code == CBATTErrorInsufficientEncryptionKeySize)
            return ErrorCodes::SimpleCommKitBlePermissionDenied;
        if (error.code == CBATTErrorReadNotPermitted || error.code == CBATTErrorWriteNotPermitted ||
            error.code == CBATTErrorRequestNotSupported) return ErrorCodes::SimpleCommKitBleNotSupported;
        return ErrorCodes::SimpleCommKitBleGattError;
    }
    if ([error.domain isEqualToString:CBErrorDomain]) {
        switch (error.code) {
        case CBErrorOperationCancelled: return ErrorCodes::SimpleCommKitBleCancelled;
        case CBErrorConnectionTimeout: case CBErrorEncryptionTimedOut: return ErrorCodes::SimpleCommKitBleTimeout;
        case CBErrorOperationNotSupported: return ErrorCodes::SimpleCommKitBleNotSupported;
        case CBErrorPeerRemovedPairingInformation: return ErrorCodes::SimpleCommKitBlePermissionDenied;
        case CBErrorNotConnected: case CBErrorPeripheralDisconnected: case CBErrorConnectionFailed: case CBErrorUnknownDevice:
            return ErrorCodes::SimpleCommKitBleBackendUnavailable;
        case CBErrorInvalidParameters: return ErrorCodes::SimpleCommKitBleInvalidArgument;
        default: break;
        }
    }
    return ErrorCodes::SimpleCommKitBleGattError;
}
std::vector<uint8_t> descriptorBytes(CBDescriptor* descriptor) {
    id value = descriptor.value;
    if (!value) return {};
    if ([value isKindOfClass:[NSData class]]) return bytes(value);
    if ([value isKindOfClass:[NSString class]]) return bytes([value dataUsingEncoding:NSUTF8StringEncoding]);
    const auto uuid = canonicalUuid(string(descriptor.UUID.UUIDString));
    if ([value isKindOfClass:[NSNumber class]] &&
        (uuid == canonicalUuid("2900") || uuid == canonicalUuid("2902") || uuid == canonicalUuid("2903"))) {
        const uint16_t n = [value unsignedShortValue];
        return {static_cast<uint8_t>(n), static_cast<uint8_t>(n >> 8)};
    }
    throw Failure(ErrorCodes::SimpleCommKitBleNotSupported, "Descriptor has no lossless byte representation");
}
} // namespace SimpleCommKit::Ble::Apple

@interface SCKBleDelegate : NSObject <CBCentralManagerDelegate, CBPeripheralDelegate> {
@public
    std::weak_ptr<SimpleCommKit::Ble::Apple::State> state;
}
@end

namespace SimpleCommKit::Ble::Apple {
enum class Kind { None, Connect, Disconnect, Services, Characteristics, Descriptors,
                  Read, ReadDescriptor, Write, WriteDescriptor, Subscribe };
struct Pending {
    Kind kind;
    __strong id object;
    __strong CBCentralManager* domain;
    std::promise<void> done;
    ErrorCode error = ErrorCodes::SimpleCommKitBleSuccess;
    std::vector<uint8_t> data;
};
struct State : std::enable_shared_from_this<State> {
    dispatch_queue_t queue = dispatch_queue_create("org.simplecommkit.ble.corebluetooth", DISPATCH_QUEUE_SERIAL);
    CBCentralManager* manager = nil;
    SCKBleDelegate* delegate = nil;
    CBCentralManager* scanManager = nil;
    SCKBleDelegate* scanDelegate = nil;
    std::function<CBCentralManager*(id<CBCentralManagerDelegate>, dispatch_queue_t)> makeManager =
        [](id<CBCentralManagerDelegate> listener, dispatch_queue_t queue) {
            return [[CBCentralManager alloc] initWithDelegate:listener queue:queue
                options:@{CBCentralManagerOptionShowPowerAlertKey: @NO}];
        };
    NSMutableDictionary<NSString*, CBPeripheral*>* devices = [NSMutableDictionary new];
    CBPeripheral* peripheral = nil;
    std::map<std::string, Peripheral> advertisements;
    std::map<std::string, bool> connectable;
    std::map<std::string, int16_t> txPower;
    std::map<std::string, std::map<std::string, std::vector<uint8_t>>> serviceData;
    std::map<CBCharacteristic*, std::pair<std::string, std::string>> subscriptions;
    EventSink sink;
    std::shared_ptr<Pending> pending;
    std::string selected;
    uint64_t generation = 0, scanGeneration = 0;
    bool closed = false, adapterSelected = false, scanActive = false, linkActive = false, discovered = false;

    template<class F> auto sync(F action) -> decltype(action()) {
        using R = decltype(action());
        // packaged_task transports C++ exceptions out of the Objective-C block.
        auto task = std::make_shared<std::packaged_task<R()>>(std::move(action));
        auto result = task->get_future();
        dispatch_sync(queue, ^{ @autoreleasepool { (*task)(); } });
        return result.get();
    }
    void emit(Event event) { if (!closed && sink) sink(std::move(event)); }
    void finish(ErrorCode code = ErrorCodes::SimpleCommKitBleSuccess, std::vector<uint8_t> data = {}) {
        if (!pending) return;
        auto request = std::move(pending);
        request->error = code;
        request->data = std::move(data);
        request->done.set_value();
    }
    bool matches(CBPeripheral* p, Kind kind, id object) const {
        return !closed && p == peripheral && pending && pending->kind == kind && pending->object == object;
    }
    void complete(CBPeripheral* p, Kind kind, id object, NSError* error) {
        if (matches(p, kind, object)) finish(errorCode(error));
    }
    void invalidate() {
        discovered = false;
        subscriptions.clear();
        if (linkActive) { linkActive = false; emit(Event(EventType::Disconnected, generation)); }
    }
    void stop() {
        const bool wasActive = scanActive;
        if (scanActive) [scanManager stopScan];
        scanActive = false;
        scanManager.delegate = nil;
        if (scanDelegate) scanDelegate->state.reset();
        scanManager = nil; scanDelegate = nil;
        if (wasActive) emit(Event(EventType::ScanStopped));
    }
    void reset() {
        // Retire the whole delegate domain after an indeterminate asynchronous
        // result. Late callbacks cannot complete an operation in a new manager.
        stop(); invalidate();
        finish(ErrorCodes::SimpleCommKitBleCancelled);
        peripheral.delegate = nil;
        if (peripheral && peripheral.state != CBPeripheralStateDisconnected)
            [manager cancelPeripheralConnection:peripheral];
        manager.delegate = nil;
        if (delegate) delegate->state.reset();
        peripheral = nil; manager = nil; delegate = nil;
        [devices removeAllObjects];
    }
    void initialize() {
        if (closed) throw Failure(ErrorCodes::SimpleCommKitBleCancelled);
        if (manager) return;
        delegate = [SCKBleDelegate new]; delegate->state = shared_from_this();
        manager = makeManager(delegate, queue);
    }
    void requireAdapter() {
        if (!adapterSelected) throw Failure(ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
    }
    void requirePeripheral(bool connection = true) {
        if (selected.empty()) throw Failure(ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
        if (connection && (!linkActive || peripheral.state != CBPeripheralStateConnected))
            throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
    }
    CBCharacteristic* characteristic(const std::string& service, const std::string& characteristic) {
        requirePeripheral();
        CBService* foundService = nil;
        for (CBService* candidate in peripheral.services) {
            if (canonicalUuid(string(candidate.UUID.UUIDString)) != service) continue;
            if (foundService) throw Failure(ErrorCodes::SimpleCommKitBleAmbiguousIdentity);
            foundService = candidate;
        }
        CBCharacteristic* found = nil;
        for (CBCharacteristic* candidate in foundService.characteristics) {
            if (canonicalUuid(string(candidate.UUID.UUIDString)) != characteristic) continue;
            if (found) throw Failure(ErrorCodes::SimpleCommKitBleAmbiguousIdentity);
            found = candidate;
        }
        if (!found) throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument);
        return found;
    }
    CBDescriptor* descriptor(CBCharacteristic* c, const std::string& uuid) {
        CBDescriptor* found = nil;
        for (CBDescriptor* candidate in c.descriptors) {
            if (canonicalUuid(string(candidate.UUID.UUIDString)) != uuid) continue;
            if (found) throw Failure(ErrorCodes::SimpleCommKitBleAmbiguousIdentity);
            found = candidate;
        }
        if (!found) throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument);
        return found;
    }
};

class Backend final : public IBleCentralBackend {
    std::shared_ptr<State> state_;
    void ready(const Operation& op, bool requirePower = true) {
        auto s = state_;
        s->sync([s] { s->initialize(); });
        for (;;) {
            op.check();
            auto status = s->sync([s] { if (s->closed) throw Failure(ErrorCodes::SimpleCommKitBleCancelled); return s->manager.state; });
            if (status != CBManagerStateUnknown && status != CBManagerStateResetting) {
                if (status == CBManagerStateUnauthorized) throw Failure(ErrorCodes::SimpleCommKitBlePermissionDenied);
                if (status == CBManagerStateUnsupported) throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
                if (requirePower && status != CBManagerStatePoweredOn) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
                return;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
    template<class F> std::vector<uint8_t> perform(const Operation& op, F start) {
        op.check(); auto s = state_;
        auto request = s->sync([s, start] {
            if (s->closed) throw Failure(ErrorCodes::SimpleCommKitBleCancelled);
            if (s->pending) throw Failure(ErrorCodes::SimpleCommKitBleReentrantOperation);
            auto p = std::make_shared<Pending>();
            p->kind = Kind::None; p->domain = s->manager;
            // Validate before installing a pending operation.
            start(p);
            return p;
        });
        auto result = request->done.get_future();
        try {
            while (result.wait_for(std::chrono::milliseconds(5)) != std::future_status::ready) op.check();
            op.check();
        } catch (...) {
            s->sync([s, request] { if (s->manager && s->manager == request->domain) s->reset(); });
            throw;
        }
        result.get();
        if (request->error != ErrorCodes::SimpleCommKitBleSuccess) {
            s->sync([s, request] { if (s->manager && s->manager == request->domain) s->reset(); });
            throw Failure(request->error);
        }
        return request->data;
    }
    void discover(const Operation& op) {
        auto s = state_;
        if (s->sync([s] { s->requirePeripheral(); return s->discovered; })) return;
        perform(op, [s](auto p) { s->requirePeripheral(); p->kind = Kind::Services; p->object = s->peripheral;
            s->pending = p; [s->peripheral discoverServices:nil]; });
        NSArray<CBService*>* services = s->sync([s] { return [s->peripheral.services copy]; });
        for (CBService* service in services) {
            perform(op, [s, service](auto p) { s->requirePeripheral(); p->kind = Kind::Characteristics; p->object = service;
                s->pending = p; [s->peripheral discoverCharacteristics:nil forService:service]; });
            NSArray<CBCharacteristic*>* characteristics = s->sync([service] { return [service.characteristics copy]; });
            for (CBCharacteristic* characteristic in characteristics)
                perform(op, [s, characteristic](auto p) { s->requirePeripheral(); p->kind = Kind::Descriptors; p->object = characteristic;
                    s->pending = p; [s->peripheral discoverDescriptorsForCharacteristic:characteristic]; });
        }
        s->sync([s] { s->requirePeripheral(); s->discovered = true; });
    }
    [[noreturn]] static void unsupported(const Operation& op) { op.check(); throw Failure(ErrorCodes::SimpleCommKitBleNotSupported); }
public:
    explicit Backend(std::shared_ptr<State> state = std::make_shared<State>()) : state_(std::move(state)) {}
    ~Backend() override { shutdown(); }
    Capabilities capabilities() const override { Capabilities c; c.tx_power = true; return c; }
    void setEventSink(EventSink sink) override { auto s = state_; s->sync([s, sink = std::move(sink)]() mutable { s->sink = std::move(sink); }); }
    void shutdown() noexcept override {
        auto s = state_;
        try { s->sync([s] { if (s->closed) return; s->closed = true; s->sink = {}; if (s->manager) s->reset(); }); } catch (...) {}
    }
    bool enabled(const Operation& op) override { ready(op, false); auto s = state_; return s->sync([s] { return s->manager.state == CBManagerStatePoweredOn; }); }
    std::vector<Adapter> adapters(const Operation& op) override {
        ready(op, false);
        return {{"corebluetooth:default", {"corebluetooth:default", ""}}};
    }
    void selectAdapter(const std::string& id, const Operation& op) override {
        op.check(); if (id != "corebluetooth:default") throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument);
        ready(op, false); auto s = state_; s->sync([s] { s->adapterSelected = true; });
    }
    bool powered(const Operation& op) override { auto s = state_; s->sync([s] { s->requireAdapter(); }); return enabled(op); }
    void setPowered(bool, const Operation& op) override { unsupported(op); }
    void startScan(const Operation& op) override {
        ready(op); auto s = state_;
        if (s->sync([s] { s->requireAdapter(); return s->scanActive; })) return;
        s->sync([s] {
            // A separate manager per scan fences old discovery callbacks even
            // when a new scan starts before the OS has drained the old one.
            s->scanDelegate = [SCKBleDelegate new]; s->scanDelegate->state = s;
            s->scanManager = s->makeManager(s->scanDelegate, s->queue);
        });
        try {
            for (;;) {
                op.check();
                const auto status = s->sync([s] { if (s->closed) throw Failure(ErrorCodes::SimpleCommKitBleCancelled); return s->scanManager.state; });
                if (status == CBManagerStatePoweredOn) break;
                if (status == CBManagerStateUnauthorized) throw Failure(ErrorCodes::SimpleCommKitBlePermissionDenied);
                if (status == CBManagerStateUnsupported) throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
                if (status == CBManagerStatePoweredOff) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
            s->sync([s, op] {
                op.check();
                s->scanGeneration = op.generation;
                s->advertisements.clear(); s->connectable.clear(); s->txPower.clear(); s->serviceData.clear();
                [s->devices removeAllObjects];
                [s->scanManager scanForPeripheralsWithServices:nil options:@{CBCentralManagerScanOptionAllowDuplicatesKey:@YES}];
                s->scanActive = true; s->emit(Event(EventType::ScanStarted));
            });
        } catch (...) { s->sync([s] { s->stop(); }); throw; }
    }
    void stopScan(const Operation& op) override { op.check(); auto s = state_; s->sync([s] { s->stop(); }); }
    bool scanning(const Operation& op) override { op.check(); auto s = state_; return s->sync([s] { return s->scanActive; }); }
    std::vector<Peripheral> peripherals(DeviceFilter filter, const Operation& op) override {
        op.check(); if (filter != DeviceFilter::All) unsupported(op);
        auto s = state_; return s->sync([s] { s->requireAdapter(); std::vector<Peripheral> out;
            for (auto& item : s->advertisements) out.push_back(item.second); return out; });
    }
    void selectPeripheral(const std::string& id, const Operation& op) override {
        op.check(); auto s = state_; s->sync([s, id] {
            s->requireAdapter(); if (!s->advertisements.count(id)) throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument);
            s->selected = id; s->peripheral = s->devices[[NSString stringWithUTF8String:id.c_str()]];
            s->discovered = false; s->subscriptions.clear();
        });
    }
    void connect(const Operation& op) override {
        ready(op); auto s = state_;
        // A new public connection generation also retires any existing session.
        s->sync([s] { s->linkActive = false; });
        disconnect(op);
        perform(op, [s, op](auto p) {
            s->requirePeripheral(false);
            NSString* key = [NSString stringWithUTF8String:s->selected.c_str()];
            NSUUID* uuid = [[NSUUID alloc] initWithUUIDString:key];
            if (!uuid) throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument);
            CBPeripheral* device = [s->manager retrievePeripheralsWithIdentifiers:@[uuid]].firstObject;
            if (!device) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
            s->devices[key] = device;
            s->peripheral = device; device.delegate = s->delegate; s->generation = op.generation;
            p->kind = Kind::Connect; p->object = device; s->pending = p;
            [s->manager connectPeripheral:device options:nil];
        });
    }
    void disconnect(const Operation& op) override {
        op.check(); auto s = state_;
        if (!s->sync([s] { s->requirePeripheral(false); return s->peripheral && s->peripheral.state != CBPeripheralStateDisconnected; })) {
            s->sync([s] { s->invalidate(); }); return;
        }
        perform(op, [s, op](auto p) { s->subscriptions.clear(); s->discovered = false; s->generation = op.generation;
            p->kind = Kind::Disconnect; p->object = s->peripheral; s->pending = p;
            [s->manager cancelPeripheralConnection:s->peripheral]; });
    }
    bool connected(const Operation& op) override { op.check(); auto s = state_; return s->sync([s] { s->requirePeripheral(false); return s->linkActive && s->peripheral.state == CBPeripheralStateConnected; }); }
    bool connectable(const Operation& op) override {
        op.check(); auto s = state_; return s->sync([s] { s->requirePeripheral(false); auto it = s->connectable.find(s->selected);
            if (it == s->connectable.end()) throw Failure(ErrorCodes::SimpleCommKitBleNotSupported); return it->second; });
    }
    bool paired(const Operation& op) override { unsupported(op); }
    void unpair(const Operation& op) override { unsupported(op); }
    int16_t txPower(const Operation& op) override {
        op.check(); auto s = state_; return s->sync([s] { s->requirePeripheral(false); auto it = s->txPower.find(s->selected);
            if (it == s->txPower.end()) throw Failure(ErrorCodes::SimpleCommKitBleNotSupported); return it->second; });
    }
    uint16_t mtu(const Operation& op) override { unsupported(op); }
    std::vector<SimpleCommKitBleService> services(const Operation& op) override {
        discover(op); auto s = state_; return s->sync([s] {
            s->requirePeripheral(); std::vector<SimpleCommKitBleService> out;
            for (CBService* service in s->peripheral.services) {
                SimpleCommKitBleService entry; entry.uuid = canonicalUuid(string(service.UUID.UUIDString));
                auto deviceData = s->serviceData.find(s->selected);
                if (deviceData != s->serviceData.end()) {
                    auto data = deviceData->second.find(entry.uuid);
                    if (data != deviceData->second.end()) entry.data = data->second;
                }
                for (CBCharacteristic* c in service.characteristics) {
                    SimpleCommKitBleCharacteristic v{}; v.uuid = canonicalUuid(string(c.UUID.UUIDString));
                    v.can_read = c.properties & CBCharacteristicPropertyRead;
                    v.can_write_request = c.properties & CBCharacteristicPropertyWrite;
                    v.can_write_command = c.properties & CBCharacteristicPropertyWriteWithoutResponse;
                    v.can_notify = c.properties & CBCharacteristicPropertyNotify;
                    v.can_indicate = c.properties & CBCharacteristicPropertyIndicate;
                    if (v.can_read) v.capabilities.push_back("read");
                    if (v.can_write_request) v.capabilities.push_back("write_request");
                    if (v.can_write_command) v.capabilities.push_back("write_command");
                    if (v.can_notify) v.capabilities.push_back("notify");
                    if (v.can_indicate) v.capabilities.push_back("indicate");
                    for (CBDescriptor* d in c.descriptors) v.descriptors_uuid.push_back(canonicalUuid(string(d.UUID.UUIDString)));
                    entry.characteristics.push_back(std::move(v));
                }
                out.push_back(std::move(entry));
            } return out;
        });
    }
    std::vector<uint8_t> read(const std::string& service, const std::string& characteristic,
        const std::string& descriptor, const Operation& op) override {
        auto su = canonicalUuid(service), cu = canonicalUuid(characteristic);
        auto du = descriptor.empty() ? "" : canonicalUuid(descriptor);
        discover(op); auto s = state_;
        return perform(op, [s, su, cu, du](auto p) {
            CBCharacteristic* c = s->characteristic(su, cu);
            if (du.empty()) {
                // CoreBluetooth cannot distinguish a read response from a
                // notification. Do not silently return notification data.
                if (!(c.properties & CBCharacteristicPropertyRead) || c.isNotifying)
                    throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
                p->kind = Kind::Read; p->object = c; s->pending = p;
                [s->peripheral readValueForCharacteristic:c];
            } else {
                CBDescriptor* d = s->descriptor(c, du); p->kind = Kind::ReadDescriptor; p->object = d; s->pending = p;
                [s->peripheral readValueForDescriptor:d];
            }
        });
    }
    void write(const std::string& service, const std::string& characteristic, const std::string& descriptor,
        const std::vector<uint8_t>& data, WriteType type, const Operation& op) override {
        auto su = canonicalUuid(service), cu = canonicalUuid(characteristic);
        auto du = descriptor.empty() ? "" : canonicalUuid(descriptor);
        discover(op); auto s = state_;
        if (du.empty() && type == WriteType::Command) {
            // Flow control is checked before submitting a command, which has no ACK.
            while (!s->sync([s] { s->requirePeripheral(); return bool(s->peripheral.canSendWriteWithoutResponse); })) {
                op.check(); std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }
        perform(op, [s, su, cu, du, data, type](auto p) {
            CBCharacteristic* c = s->characteristic(su, cu);
            NSData* value = [NSData dataWithBytes:data.data() length:data.size()];
            if (!du.empty()) {
                if (du == canonicalUuid("2902")) throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
                CBDescriptor* d = s->descriptor(c, du); p->kind = Kind::WriteDescriptor; p->object = d; s->pending = p;
                [s->peripheral writeValue:value forDescriptor:d]; return;
            }
            const bool command = type == WriteType::Command;
            const auto property = command ? CBCharacteristicPropertyWriteWithoutResponse : CBCharacteristicPropertyWrite;
            const auto nativeType = command ? CBCharacteristicWriteWithoutResponse : CBCharacteristicWriteWithResponse;
            if (!(c.properties & property)) throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
            if (data.size() > [s->peripheral maximumWriteValueLengthForType:nativeType])
                throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument);
            p->kind = Kind::Write; p->object = c; s->pending = p;
            [s->peripheral writeValue:value forCharacteristic:c type:nativeType];
            if (command) s->finish();
        });
    }
    void subscribe(const std::string& service, const std::string& characteristic, SubscriptionType type, const Operation& op) override {
        auto su = canonicalUuid(service), cu = canonicalUuid(characteristic); discover(op); auto s = state_;
        perform(op, [s, su, cu, type](auto p) {
            CBCharacteristic* c = s->characteristic(su, cu);
            const bool notify = c.properties & CBCharacteristicPropertyNotify;
            const bool indicate = c.properties & CBCharacteristicPropertyIndicate;
            if ((notify && indicate) || (type == SubscriptionType::Notify ? !notify : !indicate))
                throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
            p->kind = Kind::Subscribe; p->object = c; s->pending = p;
            s->subscriptions[c] = {su, cu}; [s->peripheral setNotifyValue:YES forCharacteristic:c];
        });
    }
    void unsubscribe(const std::string& service, const std::string& characteristic, const Operation& op) override {
        auto su = canonicalUuid(service), cu = canonicalUuid(characteristic); discover(op); auto s = state_;
        perform(op, [s, su, cu](auto p) {
            CBCharacteristic* c = s->characteristic(su, cu); s->subscriptions.erase(c);
            p->kind = Kind::Subscribe; p->object = c; s->pending = p;
            [s->peripheral setNotifyValue:NO forCharacteristic:c];
        });
    }
};
} // namespace SimpleCommKit::Ble::Apple

using namespace SimpleCommKit;

@implementation SCKBleDelegate
- (void)centralManagerDidUpdateState:(CBCentralManager*)central {
    auto s = state.lock(); if (!s || s->closed) return;
    if (central == s->scanManager) { if (central.state != CBManagerStatePoweredOn) s->stop(); return; }
    if (central != s->manager) return;
    using namespace SimpleCommKit::Ble;
    if (central.state == CBManagerStatePoweredOn) { s->emit(Event(EventType::PowerOn)); return; }
    s->stop(); s->invalidate();
    const auto code = central.state == CBManagerStateUnauthorized ? ErrorCodes::SimpleCommKitBlePermissionDenied :
        central.state == CBManagerStateUnsupported ? ErrorCodes::SimpleCommKitBleNotSupported : ErrorCodes::SimpleCommKitBleBackendUnavailable;
    s->finish(code); s->emit(Event(EventType::PowerOff));
}
- (void)centralManager:(CBCentralManager*)central didDiscoverPeripheral:(CBPeripheral*)peripheral
    advertisementData:(NSDictionary<NSString*, id>*)data RSSI:(NSNumber*)RSSI {
    auto s = state.lock(); if (!s || s->closed || central != s->scanManager || !s->scanActive) return;
    using namespace SimpleCommKit::Ble; using namespace SimpleCommKit::Ble::Apple;
    auto id = string(peripheral.identifier.UUIDString);
    Peripheral value = s->advertisements[id]; value.id = id;
    const auto name = string(data[CBAdvertisementDataLocalNameKey] ?: peripheral.name);
    if (!name.empty()) value.name = name;
    // Keep the same display convention as Windows/Linux, while carrying the
    // platform UUID in address so same-name and unnamed devices stay selectable.
    value.value.identifier = value.name;
    value.value.address = id; value.value.address_type = UNSPECIFIED;
    value.value.rssi = RSSI.shortValue;
    NSData* manufacturer = data[CBAdvertisementDataManufacturerDataKey];
    if (manufacturer.length >= 2) {
        auto payload = bytes(manufacturer);
        const uint16_t company = payload[0] | (uint16_t(payload[1]) << 8);
        value.value.manufacturer[company] = {payload.begin() + 2, payload.end()};
    }
    NSDictionary<CBUUID*, NSData*>* serviceData = data[CBAdvertisementDataServiceDataKey];
    for (CBUUID* uuid in serviceData)
        s->serviceData[id][canonicalUuid(string(uuid.UUIDString))] = bytes(serviceData[uuid]);
    if (NSNumber* connectable = data[CBAdvertisementDataIsConnectable]) s->connectable[id] = connectable.boolValue;
    if (NSNumber* power = data[CBAdvertisementDataTxPowerLevelKey]) s->txPower[id] = power.shortValue;
    s->devices[peripheral.identifier.UUIDString] = peripheral; s->advertisements[id] = value;
    Event event(EventType::Advertisement, s->scanGeneration); event.peripheral = std::move(value); s->emit(std::move(event));
}
- (void)centralManager:(CBCentralManager*)central didConnectPeripheral:(CBPeripheral*)peripheral {
    auto s = state.lock(); if (!s || central != s->manager) return;
    using namespace SimpleCommKit::Ble; using namespace SimpleCommKit::Ble::Apple;
    if (!s->matches(peripheral, Kind::Connect, peripheral)) { [central cancelPeripheralConnection:peripheral]; return; }
    s->linkActive = true; s->discovered = false;
    s->emit(Event(EventType::Connected, s->generation)); s->finish();
}
- (void)centralManager:(CBCentralManager*)central didFailToConnectPeripheral:(CBPeripheral*)peripheral error:(NSError*)error {
    auto s = state.lock(); if (!s || central != s->manager) return;
    using namespace SimpleCommKit::Ble; using namespace SimpleCommKit::Ble::Apple;
    if (s->matches(peripheral, Kind::Connect, peripheral))
        s->finish(error ? errorCode(error) : ErrorCodes::SimpleCommKitBleBackendUnavailable);
}
- (void)centralManager:(CBCentralManager*)central didDisconnectPeripheral:(CBPeripheral*)peripheral error:(NSError*)error {
    (void)error;
    auto s = state.lock(); if (!s || central != s->manager || peripheral != s->peripheral) return;
    using namespace SimpleCommKit::Ble; using namespace SimpleCommKit::Ble::Apple;
    const bool expected = s->pending && s->pending->kind == Kind::Disconnect;
    s->invalidate(); s->finish(expected ? ErrorCodes::SimpleCommKitBleSuccess : ErrorCodes::SimpleCommKitBleBackendUnavailable);
}
- (void)peripheral:(CBPeripheral*)peripheral didDiscoverServices:(NSError*)error {
    auto s = state.lock(); if (s) s->complete(peripheral, SimpleCommKit::Ble::Apple::Kind::Services, peripheral, error);
}
- (void)peripheral:(CBPeripheral*)peripheral didDiscoverCharacteristicsForService:(CBService*)service error:(NSError*)error {
    auto s = state.lock(); if (s) s->complete(peripheral, SimpleCommKit::Ble::Apple::Kind::Characteristics, service, error);
}
- (void)peripheral:(CBPeripheral*)peripheral didDiscoverDescriptorsForCharacteristic:(CBCharacteristic*)characteristic error:(NSError*)error {
    auto s = state.lock(); if (s) s->complete(peripheral, SimpleCommKit::Ble::Apple::Kind::Descriptors, characteristic, error);
}
- (void)peripheral:(CBPeripheral*)peripheral didUpdateValueForCharacteristic:(CBCharacteristic*)characteristic error:(NSError*)error {
    auto s = state.lock(); if (!s || s->closed || peripheral != s->peripheral) return;
    using namespace SimpleCommKit::Ble; using namespace SimpleCommKit::Ble::Apple;
    if (s->matches(peripheral, Kind::Read, characteristic)) { s->finish(errorCode(error), bytes(characteristic.value)); return; }
    auto it = s->subscriptions.find(characteristic); if (it == s->subscriptions.end() || !s->linkActive) return;
    if (error) { Event event(EventType::Error, s->generation); event.error = errorCode(error); s->emit(std::move(event)); return; }
    Event event(EventType::Notification, s->generation); event.service = it->second.first; event.characteristic = it->second.second;
    event.data = bytes(characteristic.value); s->emit(std::move(event));
}
- (void)peripheral:(CBPeripheral*)peripheral didUpdateValueForDescriptor:(CBDescriptor*)descriptor error:(NSError*)error {
    auto s = state.lock(); if (!s) return;
    using namespace SimpleCommKit::Ble; using namespace SimpleCommKit::Ble::Apple;
    if (!s->matches(peripheral, Kind::ReadDescriptor, descriptor)) return;
    if (error) { s->finish(errorCode(error)); return; }
    try { s->finish(ErrorCodes::SimpleCommKitBleSuccess, descriptorBytes(descriptor)); }
    catch (const Failure& failure) { s->finish(failure.code); }
}
- (void)peripheral:(CBPeripheral*)peripheral didWriteValueForCharacteristic:(CBCharacteristic*)characteristic error:(NSError*)error {
    auto s = state.lock(); if (s) s->complete(peripheral, SimpleCommKit::Ble::Apple::Kind::Write, characteristic, error);
}
- (void)peripheral:(CBPeripheral*)peripheral didWriteValueForDescriptor:(CBDescriptor*)descriptor error:(NSError*)error {
    auto s = state.lock(); if (s) s->complete(peripheral, SimpleCommKit::Ble::Apple::Kind::WriteDescriptor, descriptor, error);
}
- (void)peripheral:(CBPeripheral*)peripheral didUpdateNotificationStateForCharacteristic:(CBCharacteristic*)characteristic error:(NSError*)error {
    auto s = state.lock(); if (!s || peripheral != s->peripheral) return;
    if (error || !characteristic.isNotifying) s->subscriptions.erase(characteristic);
    s->complete(peripheral, SimpleCommKit::Ble::Apple::Kind::Subscribe, characteristic, error);
}
- (void)peripheral:(CBPeripheral*)peripheral didModifyServices:(NSArray<CBService*>*)services {
    (void)services;
    auto s = state.lock(); if (!s || peripheral != s->peripheral) return;
    using namespace SimpleCommKit::Ble;
    s->discovered = false; s->subscriptions.clear();
    s->finish(ErrorCodes::SimpleCommKitBleGattError);
    Event event(EventType::Error, s->generation); event.error = ErrorCodes::SimpleCommKitBleGattError; s->emit(std::move(event));
    // Retire the session to fence responses for the invalidated GATT objects.
    s->reset();
}
@end

namespace SimpleCommKit::Ble {
std::unique_ptr<IBleCentralBackend> createBackend() { return std::make_unique<Apple::Backend>(); }
} // namespace SimpleCommKit::Ble
