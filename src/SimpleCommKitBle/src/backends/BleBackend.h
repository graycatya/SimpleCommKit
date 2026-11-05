#pragma once

#include "SimpleCommKitBleCentral.h"
#include <atomic>
#include <chrono>
#include <stdexcept>

namespace SimpleCommKit::Ble {
using Clock = std::chrono::steady_clock;
inline constexpr auto OperationTimeout = std::chrono::seconds(30);

class Failure final : public std::runtime_error {
public:
    explicit Failure(ErrorCode code, const std::string& message = "BLE operation failed")
        : std::runtime_error(message), code(code) {}
    ErrorCode code;
};

// One context per operation; a connection generation is never reused.
struct Operation {
    Clock::time_point deadline = Clock::now() + OperationTimeout;
    std::shared_ptr<std::atomic<bool>> cancelled = std::make_shared<std::atomic<bool>>(false);
    uint64_t generation = 0;
    void check() const {
        if (cancelled->load()) throw Failure(ErrorCodes::SimpleCommKitBleCancelled);
        if (Clock::now() >= deadline) throw Failure(ErrorCodes::SimpleCommKitBleTimeout);
    }
};

struct Capabilities {
    bool power_control = false;
    bool paired_devices = false;
    bool pairing_state = false;
    bool unpair = false;
    bool mtu = false;
    bool tx_power = false;
    bool multiple_adapters = false;
};
struct Adapter {
    std::string id; // Stable native key; never a display name.
    SimpleCommKitBleAdapter value;
};
struct Peripheral {
    std::string id;
    std::string name;
    SimpleCommKitBlePeripheral value{};
};
enum class EventType { PowerOn, PowerOff, ScanStarted, ScanStopped, Advertisement,
                       Connected, Disconnected, Notification, Error };
struct Event {
    explicit Event(EventType type, uint64_t generation = 0) : type(type), generation(generation) {}
    EventType type;
    uint64_t generation = 0;
    Peripheral peripheral;
    std::string service, characteristic;
    std::vector<uint8_t> data;
    ErrorCode error = ErrorCodes::SimpleCommKitBleSuccess;
    uint64_t subscriptionRevision = 0;
};
using EventSink = std::function<void(Event)>;
enum class DeviceFilter { All, Paired, Connected };
enum class WriteType { Request, Command };
enum class SubscriptionType { Notify, Indicate };

// Private contract. Implementations throw Failure once; the facade alone reports
// operation errors to users. System callbacks only emit values, never raw this.
// All methods run on the operation executor except setEventSink and shutdown.
class IBleCentralBackend {
public:
    virtual ~IBleCentralBackend() = default;
    virtual Capabilities capabilities() const = 0;
    virtual void setEventSink(EventSink sink) = 0;
    // Idempotent, thread safe, cancels I/O and unbinds listeners; no user code.
    virtual void shutdown() noexcept = 0;
    virtual bool enabled(const Operation&) = 0;
    virtual std::vector<Adapter> adapters(const Operation&) = 0;
    virtual void selectAdapter(const std::string& id, const Operation&) = 0;
    virtual bool powered(const Operation&) = 0;
    virtual void setPowered(bool, const Operation&) = 0;
    virtual void startScan(const Operation&) = 0;
    virtual void stopScan(const Operation&) = 0;
    virtual bool scanning(const Operation&) = 0;
    virtual std::vector<Peripheral> peripherals(DeviceFilter, const Operation&) = 0;
    virtual void selectPeripheral(const std::string& id, const Operation&) = 0;
    virtual void connect(const Operation&) = 0;
    virtual void disconnect(const Operation&) = 0;
    virtual bool connected(const Operation&) = 0;
    virtual bool connectable(const Operation&) = 0;
    virtual bool paired(const Operation&) = 0;
    virtual void unpair(const Operation&) = 0;
    virtual int16_t txPower(const Operation&) = 0;
    virtual uint16_t mtu(const Operation&) = 0;
    virtual std::vector<SimpleCommKitBleService> services(const Operation&) = 0;
    // Empty descriptor selects a characteristic. UUID ambiguity must fail.
    virtual std::vector<uint8_t> read(const std::string& service,
        const std::string& characteristic, const std::string& descriptor, const Operation&) = 0;
    virtual void write(const std::string& service, const std::string& characteristic,
        const std::string& descriptor, const std::vector<uint8_t>&, WriteType, const Operation&) = 0;
    virtual void subscribe(const std::string& service, const std::string& characteristic,
        SubscriptionType, const Operation&) = 0;
    virtual void unsubscribe(const std::string& service, const std::string& characteristic,
        const Operation&) = 0;
};
std::unique_ptr<IBleCentralBackend> createBackend();
std::string canonicalUuid(const std::string& uuid);
} // namespace SimpleCommKit::Ble
