// White-box delegate integration tests use Objective-C stand-ins. No manager is
// constructed, no permission prompt appears, and no Bluetooth radio is accessed.
#include "../../src/SimpleCommKitBle/src/backends/apple/CoreBluetoothBackend.mm"
#include <iostream>
#include <cstdlib>
using namespace SimpleCommKit;
using namespace SimpleCommKit::Ble;
using namespace SimpleCommKit::Ble::Apple;
static void check(bool condition, const char* message) {
    if (!condition) { std::cerr << message << '\n'; std::exit(1); }
}
template<class F> void fails(ErrorCode code, F action) {
    try { action(); } catch (const Failure& failure) { if (failure.code != code) std::cerr << "expected " << code << ", got " << failure.code << "\n"; check(failure.code == code, "wrong failure code"); return; }
    check(false, "expected failure");
}
@interface SCKTestDescriptor : NSObject
@property(nonatomic, strong) CBUUID* UUID;
@property(nonatomic, strong) id value;
@end
@implementation SCKTestDescriptor
@end
@interface SCKTestCharacteristic : NSObject
@property(nonatomic, strong) CBUUID* UUID;
@property(nonatomic) CBCharacteristicProperties properties;
@property(nonatomic, strong) NSArray* descriptors;
@property(nonatomic, strong) NSData* value;
@property(nonatomic, getter=isNotifying) BOOL notifying;
@end
@implementation SCKTestCharacteristic
@end
@interface SCKTestService : NSObject
@property(nonatomic, strong) CBUUID* UUID;
@property(nonatomic, strong) NSArray* characteristics;
@end
@implementation SCKTestService
@end
@interface SCKTestPeripheral : NSObject
@property(nonatomic, weak) id<CBPeripheralDelegate> delegate;
@property(nonatomic) CBPeripheralState state;
@property(nonatomic, strong) NSUUID* identifier;
@property(nonatomic, strong) NSString* name;
@property(nonatomic, strong) NSArray* services;
@property(nonatomic) BOOL canSendWriteWithoutResponse;
@property(nonatomic) BOOL holdRead;
@property(nonatomic, strong) NSData* lastWrite;
@end
@implementation SCKTestPeripheral
- (void)discoverServices:(NSArray*)UUIDs {
    (void)UUIDs; [self.delegate peripheral:(CBPeripheral*)(id)self didDiscoverServices:nil];
}
- (void)discoverCharacteristics:(NSArray*)UUIDs forService:(CBService*)service {
    (void)UUIDs; [self.delegate peripheral:(CBPeripheral*)(id)self didDiscoverCharacteristicsForService:service error:nil];
}
- (void)discoverDescriptorsForCharacteristic:(CBCharacteristic*)c {
    [self.delegate peripheral:(CBPeripheral*)(id)self didDiscoverDescriptorsForCharacteristic:c error:nil];
}
- (void)readValueForCharacteristic:(CBCharacteristic*)c {
    if (!self.holdRead) [self.delegate peripheral:(CBPeripheral*)(id)self didUpdateValueForCharacteristic:c error:nil];
}
- (void)readValueForDescriptor:(CBDescriptor*)d {
    [self.delegate peripheral:(CBPeripheral*)(id)self didUpdateValueForDescriptor:d error:nil];
}
- (NSUInteger)maximumWriteValueLengthForType:(CBCharacteristicWriteType)type { (void)type; return 20; }
- (void)writeValue:(NSData*)data forCharacteristic:(CBCharacteristic*)c type:(CBCharacteristicWriteType)type {
    self.lastWrite = data;
    if (type == CBCharacteristicWriteWithResponse)
        [self.delegate peripheral:(CBPeripheral*)(id)self didWriteValueForCharacteristic:c error:nil];
}
- (void)writeValue:(NSData*)data forDescriptor:(CBDescriptor*)d {
    self.lastWrite = data; [self.delegate peripheral:(CBPeripheral*)(id)self didWriteValueForDescriptor:d error:nil];
}
- (void)setNotifyValue:(BOOL)enabled forCharacteristic:(CBCharacteristic*)c {
    [(SCKTestCharacteristic*)(id)c setNotifying:enabled];
    [self.delegate peripheral:(CBPeripheral*)(id)self didUpdateNotificationStateForCharacteristic:c error:nil];
}
@end
@interface SCKTestManager : NSObject
@property(nonatomic, weak) id<CBCentralManagerDelegate> delegate;
@property(nonatomic) CBManagerState state;
@property(nonatomic, strong) NSArray* known;
@end
@implementation SCKTestManager
- (void)scanForPeripheralsWithServices:(NSArray*)services options:(NSDictionary*)options { (void)services; (void)options; }
- (void)stopScan {}
- (NSArray*)retrievePeripheralsWithIdentifiers:(NSArray*)ids { (void)ids; return self.known; }
- (void)connectPeripheral:(CBPeripheral*)p options:(NSDictionary*)options {
    (void)options; [(SCKTestPeripheral*)(id)p setState:CBPeripheralStateConnected];
    [self.delegate centralManager:(CBCentralManager*)(id)self didConnectPeripheral:p];
}
- (void)cancelPeripheralConnection:(CBPeripheral*)p {
    [(SCKTestPeripheral*)(id)p setState:CBPeripheralStateDisconnected];
    [self.delegate centralManager:(CBCentralManager*)(id)self didDisconnectPeripheral:p error:nil];
}
@end

static void simulatedSession(int ending) {
    auto s = std::make_shared<State>();
    SCKTestManager* manager = [SCKTestManager new]; manager.state = CBManagerStatePoweredOn;
    SCKBleDelegate* delegate = [SCKBleDelegate new]; delegate->state = s; manager.delegate = delegate;
    s->manager = (CBCentralManager*)(id)manager; s->delegate = delegate;
    s->makeManager = [](id<CBCentralManagerDelegate> listener, dispatch_queue_t queue) {
        (void)queue; SCKTestManager* fake = [SCKTestManager new]; fake.delegate = listener; fake.state = CBManagerStatePoweredOn;
        return (CBCentralManager*)(id)fake;
    };
    Backend backend(s); Operation op; op.generation = 42;
    std::vector<Event> events;
    backend.setEventSink([&](Event event) { events.push_back(std::move(event)); });
    check(backend.adapters(op).size() == 1, "default adapter");
    s->sync([&] { manager.state = CBManagerStateUnauthorized; });
    fails(ErrorCodes::SimpleCommKitBlePermissionDenied, [&] { backend.adapters(op); });
    s->sync([&] { manager.state = CBManagerStatePoweredOff; });
    check(!backend.enabled(op), "powered off query");
    fails(ErrorCodes::SimpleCommKitBleBackendUnavailable, [&] { backend.startScan(op); });
    s->sync([&] { manager.state = CBManagerStatePoweredOn; });
    backend.selectAdapter("corebluetooth:default", op);
    backend.startScan(op);
    SCKTestPeripheral* peripheral = [SCKTestPeripheral new];
    peripheral.identifier = [[NSUUID alloc] initWithUUIDString:@"01234567-89AB-CDEF-0123-456789ABCDEF"];
    peripheral.name = @"Test device"; peripheral.canSendWriteWithoutResponse = YES;
    SCKTestDescriptor* descriptor = [SCKTestDescriptor new]; descriptor.UUID = [CBUUID UUIDWithString:@"2902"]; descriptor.value = @1;
    SCKTestCharacteristic* characteristic = [SCKTestCharacteristic new]; characteristic.UUID = [CBUUID UUIDWithString:@"2A19"];
    characteristic.properties = CBCharacteristicPropertyRead | CBCharacteristicPropertyWrite | CBCharacteristicPropertyWriteWithoutResponse | CBCharacteristicPropertyNotify;
    characteristic.descriptors = @[descriptor]; characteristic.value = [NSData dataWithBytes:"\0\xff\x23" length:3];
    SCKTestService* service = [SCKTestService new]; service.UUID = [CBUUID UUIDWithString:@"180F"]; service.characteristics = @[characteristic];
    peripheral.services = @[service]; manager.known = @[peripheral];
    SCKBleDelegate* scanDelegate = s->scanDelegate;
    CBCentralManager* scanManager = s->scanManager;
    auto advertise = [&] { s->sync([&] {
        [scanDelegate centralManager:scanManager didDiscoverPeripheral:(CBPeripheral*)(id)peripheral
            advertisementData:@{CBAdvertisementDataLocalNameKey:@"Test device", CBAdvertisementDataIsConnectable:@YES,
                CBAdvertisementDataTxPowerLevelKey:@(-7), CBAdvertisementDataManufacturerDataKey:[NSData dataWithBytes:"\x34\x12\0\xff" length:4]}
            RSSI:@(-45)];
    }); };
    advertise(); advertise();
    auto found = backend.peripherals(DeviceFilter::All, op);
    check(found.size() == 1 && found[0].value.identifier == "Test device" &&
        found[0].value.address == string(peripheral.identifier.UUIDString) &&
        found[0].value.address_type == UNSPECIFIED, "display name, platform address and deduplication");
    s->sync([&] {
        peripheral.name = nil;
        [scanDelegate centralManager:scanManager didDiscoverPeripheral:(CBPeripheral*)(id)peripheral advertisementData:@{} RSSI:@(-46)];
    });
    check(backend.peripherals(DeviceFilter::All, op)[0].value.identifier == "Test device", "preserve name across nameless updates");
    SCKTestPeripheral* sameName = [SCKTestPeripheral new]; sameName.name = @"Test device";
    sameName.identifier = [[NSUUID alloc] initWithUUIDString:@"11234567-89AB-CDEF-0123-456789ABCDEF"];
    SCKTestPeripheral* unnamed = [SCKTestPeripheral new];
    unnamed.identifier = [[NSUUID alloc] initWithUUIDString:@"21234567-89AB-CDEF-0123-456789ABCDEF"];
    s->sync([&] {
        [scanDelegate centralManager:scanManager didDiscoverPeripheral:(CBPeripheral*)(id)sameName advertisementData:@{} RSSI:@(-50)];
        [scanDelegate centralManager:scanManager didDiscoverPeripheral:(CBPeripheral*)(id)unnamed advertisementData:@{} RSSI:@(-60)];
    });
    auto identities = backend.peripherals(DeviceFilter::All, op);
    check(identities.size() == 3 && identities[0].value.identifier == identities[1].value.identifier &&
        identities[0].value.address != identities[1].value.address && identities[2].value.identifier.empty() &&
        !identities[2].value.address.empty(), "same-name and unnamed devices retain distinct UUID addresses");
    check(found[0].value.manufacturer.at(0x1234) == std::vector<uint8_t>({0,255}), "manufacturer bytes");
    s->sync([&] { check(events.back().generation == 42, "scan generation"); });
    backend.stopScan(op); backend.stopScan(op);
    const auto count = s->sync([&] { return events.size(); }); advertise();
    s->sync([&] { check(events.size() == count, "late advertisement after stop"); });
    backend.startScan(op);
    const auto restartedCount = s->sync([&] { return events.size(); }); advertise();
    s->sync([&] { check(events.size() == restartedCount, "old scan domain suppressed after restart");
        [s->scanDelegate centralManager:s->scanManager didDiscoverPeripheral:(CBPeripheral*)(id)peripheral advertisementData:@{CBAdvertisementDataIsConnectable:@YES, CBAdvertisementDataTxPowerLevelKey:@(-7), CBAdvertisementDataServiceDataKey:@{[CBUUID UUIDWithString:@"180F"]:[NSData dataWithBytes:"\x07\0" length:2]}} RSSI:@(-45)]; });
    backend.stopScan(op);
    backend.selectPeripheral(found[0].id, op);
    check(backend.connectable(op) && backend.txPower(op) == -7, "advertised properties");
    backend.connect(op); check(backend.connected(op), "connected");
    auto services = backend.services(op);
    check(services.size() == 1 && services[0].characteristics[0].can_write_command, "GATT capabilities");
    check(services[0].data == std::vector<uint8_t>({7,0}), "advertised service data");
    check(backend.read("180f", "2a19", "", op) == std::vector<uint8_t>({0,255,35}), "binary read");
    check(backend.read("180f", "2a19", "2902", op) == std::vector<uint8_t>({1,0}), "typed descriptor little endian");
    backend.write("180f", "2a19", "", {1,0,255}, WriteType::Request, op);
    check(bytes(peripheral.lastWrite) == std::vector<uint8_t>({1,0,255}), "request payload");
    backend.write("180f", "2a19", "", {9}, WriteType::Command, op);
    fails(ErrorCodes::SimpleCommKitBleInvalidArgument, [&] { backend.write("180f", "2a19", "", std::vector<uint8_t>(21), WriteType::Request, op); });
    fails(ErrorCodes::SimpleCommKitBleNotSupported, [&] { backend.write("180f", "2a19", "2902", {1,0}, WriteType::Request, op); });
    s->sync([&] { service.characteristics = @[characteristic, characteristic]; });
    fails(ErrorCodes::SimpleCommKitBleAmbiguousIdentity, [&] { backend.read("180f", "2a19", "", op); });
    s->sync([&] { service.characteristics = @[characteristic]; });
    backend.subscribe("180f", "2a19", SubscriptionType::Notify, op);
    fails(ErrorCodes::SimpleCommKitBleNotSupported, [&] { backend.read("180f", "2a19", "", op); });
    s->sync([&] {
        [delegate peripheral:(CBPeripheral*)(id)peripheral didUpdateValueForCharacteristic:(CBCharacteristic*)(id)characteristic error:nil];
        check(events.back().type == EventType::Notification && events.back().data == std::vector<uint8_t>({0,255,35}), "notification payload");
    });
    backend.unsubscribe("180f", "2a19", op);
    const auto afterUnsubscribe = s->sync([&] { return events.size(); });
    s->sync([&] { [delegate peripheral:(CBPeripheral*)(id)peripheral didUpdateValueForCharacteristic:(CBCharacteristic*)(id)characteristic error:nil];
        check(events.size() == afterUnsubscribe, "late notification suppressed");
        characteristic.properties |= CBCharacteristicPropertyIndicate; });
    fails(ErrorCodes::SimpleCommKitBleNotSupported, [&] { backend.subscribe("180f", "2a19", SubscriptionType::Indicate, op); });
    s->sync([&] { characteristic.properties &= ~CBCharacteristicPropertyNotify; });
    backend.subscribe("180f", "2a19", SubscriptionType::Indicate, op);
    backend.unsubscribe("180f", "2a19", op);
    Operation disconnectOp; disconnectOp.generation = 43;
    backend.disconnect(disconnectOp); backend.disconnect(disconnectOp);
    s->sync([&] { check(events.back().type == EventType::Disconnected && events.back().generation == 43, "explicit disconnect generation"); });
    backend.connect(op);
    // A real missed delegate response must time out and retire all listeners.
    s->sync([&] { peripheral.holdRead = YES; });
    Operation shortOp; shortOp.generation = 42;
    shortOp.deadline = Clock::now() + std::chrono::milliseconds(ending == 0 ? 80 : 1000);
    std::thread interrupt;
    if (ending != 0) interrupt = std::thread([&, ending] {
        std::this_thread::sleep_for(std::chrono::milliseconds(30));
        if (ending == 1) shortOp.cancelled->store(true);
        if (ending == 2) backend.shutdown();
        if (ending == 3) s->sync([&] { [delegate peripheral:(CBPeripheral*)(id)peripheral didModifyServices:@[]]; });
    });
    const auto expected = ending == 0 ? ErrorCodes::SimpleCommKitBleTimeout :
        ending == 3 ? ErrorCodes::SimpleCommKitBleGattError : ErrorCodes::SimpleCommKitBleCancelled;
    fails(expected, [&] { backend.read("180f", "2a19", "", shortOp); });
    if (interrupt.joinable()) interrupt.join();
    s->sync([&] { check(!s->manager && !s->peripheral && s->subscriptions.empty(), "timeout domain retired"); });
    check(delegate->state.expired(), "old delegate detached");
    const auto retiredCount = s->sync([&] { return events.size(); });
    s->sync([&] { [delegate centralManager:(CBCentralManager*)(id)manager didConnectPeripheral:(CBPeripheral*)(id)peripheral];
        check(events.size() == retiredCount, "late connection suppressed"); });
    backend.shutdown(); backend.shutdown();
}

int main(int argc, char** argv) { @autoreleasepool {
    if (argc > 1 && std::string(argv[1]) == "--scan") {
        auto backend = createBackend(); Operation op; op.generation = 1;
        backend->setEventSink([](Event event) {
            if (event.type == EventType::Advertisement)
                std::cout << event.peripheral.value.identifier << " (" << event.peripheral.value.address << ") RSSI=" << event.peripheral.value.rssi << '\n';
        });
        try {
            auto adapters = backend->adapters(op); check(!adapters.empty(), "no adapter");
            backend->selectAdapter(adapters.front().id, op); backend->startScan(op);
            std::this_thread::sleep_for(std::chrono::seconds(argc > 2 ? std::stoi(argv[2]) : 5)); backend->stopScan(op);
            std::cout << "Devices: " << backend->peripherals(DeviceFilter::All, op).size() << '\n';
        } catch (const Failure& failure) { std::cerr << "BLE error " << failure.code << ": " << failure.what() << '\n'; return 2; }
        backend->shutdown(); return 0;
    }
    check(errorCode([NSError errorWithDomain:CBATTErrorDomain code:CBATTErrorInsufficientAuthentication userInfo:nil]) == ErrorCodes::SimpleCommKitBlePermissionDenied, "permission mapping");
    check(errorCode([NSError errorWithDomain:CBErrorDomain code:CBErrorConnectionTimeout userInfo:nil]) == ErrorCodes::SimpleCommKitBleTimeout, "timeout mapping");
    auto backend = createBackend(); Operation op;
    fails(ErrorCodes::SimpleCommKitBleNotSupported, [&] { backend->mtu(op); });
    fails(ErrorCodes::SimpleCommKitBleNotSupported, [&] { backend->paired(op); });
    fails(ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet, [&] { backend->connected(op); });
    backend->shutdown(); backend->shutdown();
    for (int ending = 0; ending < 4; ++ending) simulatedSession(ending);
    std::cout << "CoreBluetooth simulated session passed\n";
    return 0;
} }
