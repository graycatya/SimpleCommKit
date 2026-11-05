#pragma once
#include "BleRuntime.h"
namespace SimpleCommKit::Ble {
// Shared test controls outlive backend destruction to inject late system events.
struct FakeControl {
    std::mutex mutex;
    EventSink sink;
    std::atomic<uint64_t> generation{1};
    std::atomic<bool> scanning{false}, connected{false}, stopped{false}, blockRead{false}, reading{false};
    std::vector<Peripheral> devices;
    void emit(Event event) { EventSink copy;{std::lock_guard<std::mutex> lock(mutex);copy=sink;}if(copy)copy(std::move(event)); }
    void event(EventType type){emit(Event{type,generation.load()});}
    void advertise(const Peripheral& p){Event e{EventType::Advertisement,generation.load()};e.peripheral=p;emit(std::move(e));}
};
inline std::shared_ptr<FakeControl> fake;
class FakeBackend final : public IBleCentralBackend {
    std::shared_ptr<FakeControl> control_;
    bool selected_=false;
public:
    FakeBackend():control_(std::make_shared<FakeControl>()){fake=control_;}
    Capabilities capabilities()const override{return {};}
    void setEventSink(EventSink sink)override{std::lock_guard<std::mutex> lock(control_->mutex);control_->sink=std::move(sink);}
    void shutdown()noexcept override{control_->stopped=true;setEventSink({});}
    bool enabled(const Operation& o)override{o.check();return true;}
    std::vector<Adapter> adapters(const Operation&)override{return {{"adapter",{"adapter","01"}}};}
    void selectAdapter(const std::string&,const Operation& o)override{control_->generation=o.generation;}
    bool powered(const Operation&)override{return true;}
    void setPowered(bool,const Operation&)override{throw Failure(ErrorCodes::SimpleCommKitBlePermissionDenied);}
    void startScan(const Operation& o)override{control_->generation=o.generation;if(!control_->scanning.exchange(true))control_->event(EventType::ScanStarted);}
    void stopScan(const Operation& o)override{control_->generation=o.generation;if(control_->scanning.exchange(false))control_->event(EventType::ScanStopped);}
    bool scanning(const Operation&)override{return control_->scanning;}
    std::vector<Peripheral> peripherals(DeviceFilter,const Operation&)override{return control_->devices;}
    void selectPeripheral(const std::string&,const Operation& o)override{selected_=true;control_->generation=o.generation;}
    void connect(const Operation& o)override{if(!selected_)throw Failure(ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);control_->generation=o.generation;control_->connected=true;control_->event(EventType::Connected);}
    void disconnect(const Operation& o)override{control_->generation=o.generation;control_->connected=false;control_->event(EventType::Disconnected);}
    bool connected(const Operation&)override{return control_->connected;}
    bool connectable(const Operation&)override{return selected_;}
    bool paired(const Operation&)override{return false;}
    void unpair(const Operation&)override{throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);}
    int16_t txPower(const Operation&)override{return -7;}
    uint16_t mtu(const Operation&)override{return 247;}
    std::vector<SimpleCommKitBleService> services(const Operation&)override{return {};}
    std::vector<uint8_t> read(const std::string&,const std::string&,const std::string&,const Operation& o)override{
        control_->reading=true;while(control_->blockRead){o.check();std::this_thread::sleep_for(std::chrono::milliseconds(1));}return {0,1,255};
    }
    void write(const std::string&,const std::string&,const std::string&,const std::vector<uint8_t>&,WriteType,const Operation&)override{}
    void subscribe(const std::string&,const std::string&,SubscriptionType,const Operation&)override{}
    void unsubscribe(const std::string&,const std::string&,const Operation&)override{}
};
} // namespace SimpleCommKit::Ble
