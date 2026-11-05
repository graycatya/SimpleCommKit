#include "BleRuntime.h"
#include <set>

namespace SimpleCommKit {
using namespace Ble;
namespace {
struct CentralState : std::enable_shared_from_this<CentralState> {
    std::shared_ptr<IBleCentralBackend> backend;
    SerialExecutor operations;
    SerialExecutor events;
    Generation generation;
    std::mutex mutex;
    bool closing = false, scanActive = false, connectionActive = false;
    std::optional<Adapter> adapter;
    std::optional<Peripheral> selected;
    std::map<std::string, Peripheral> discovered;
    std::map<EventType, std::function<void()>> callbacks;
    std::function<void(SimpleCommKitBlePeripheral)> found, updated;
    std::function<void(ErrorCode)> error;
    struct Subscription { uint64_t revision; std::function<void(std::vector<uint8_t>)> callback; };
    uint64_t revision = 0, requestedDisconnectGeneration = 0;
    using SubscriptionMap=std::map<std::pair<std::string,std::string>, Subscription>;
    SubscriptionMap subscriptions;
    void clearSubscriptions() {
        SubscriptionMap discarded;
        {std::lock_guard<std::mutex> lock(mutex); discarded.swap(subscriptions);}
    }

    explicit CentralState(std::unique_ptr<IBleCentralBackend> b) : backend(std::move(b)) {}
    void initialize() {
        std::weak_ptr<CentralState> weak = shared_from_this();
        backend->setEventSink([weak](Event e) {
            if (auto state = weak.lock()) {
                if(e.type==EventType::Notification) {
                    std::lock_guard<std::mutex> lock(state->mutex);
                    auto it=state->subscriptions.find({e.service,e.characteristic});
                    if(it==state->subscriptions.end())return;
                    e.subscriptionRevision=it->second.revision;
                }
                state->events.post([weak,e=std::move(e)]() mutable {
                if (auto state = weak.lock()) state->handle(std::move(e));
                });
            }
        });
    }
    void shutdown() {
        generation.close();
        decltype(callbacks) discardedCallbacks;
        decltype(found) discardedFound, discardedUpdated;
        decltype(error) discardedError;
        SubscriptionMap discardedSubscriptions;
        {
            std::lock_guard<std::mutex> lock(mutex);
            closing = true; callbacks.swap(discardedCallbacks);
            found.swap(discardedFound);updated.swap(discardedUpdated);error.swap(discardedError);
            subscriptions.swap(discardedSubscriptions);
        }
        backend->setEventSink({});
        events.finish();
        // Cancelled calls finish before backend teardown.
        operations.finish();
        backend->shutdown();
    }
    void report(ErrorCode code) {
        std::weak_ptr<CentralState> weak=shared_from_this();
        events.post([weak,code] {
            if(auto state=weak.lock()) {
                std::function<void(ErrorCode)> callback;
                {std::lock_guard<std::mutex> lock(state->mutex);if(!state->closing) callback=state->error;}
                if(callback) callback(code);
            }
        });
    }
    template<class F> auto call(F work, bool callbackSafe=false) -> decltype(work(std::declval<IBleCentralBackend&>(),std::declval<const Operation&>())) {
        using Result=decltype(work(std::declval<IBleCentralBackend&>(),std::declval<const Operation&>()));
        try {
            if(events.isCurrent() && !callbackSafe) throw Failure(ErrorCodes::SimpleCommKitBleReentrantOperation);
            auto op=generation.begin(); auto b=backend;
            return waitFor(operations,op,[b,work=std::move(work)](const Operation& operation) mutable {return work(*b,operation);});
        } catch(const Failure& e) {report(e.code);}
          catch(...) {report(ErrorCodes::SimpleCommKitBleGattError);}
        if constexpr(!std::is_void_v<Result>) return Result{};
    }
    void request(std::function<void(IBleCentralBackend&,const Operation&)> work) {
        try {
            auto op=generation.begin(); auto b=backend; std::weak_ptr<CentralState> weak=shared_from_this();
            operations.post([b,op,weak,work=std::move(work)] {
                try {op.check();work(*b,op);} catch(const Failure& e){if(auto s=weak.lock())s->report(e.code);}
                catch(...){if(auto s=weak.lock())s->report(ErrorCodes::SimpleCommKitBleGattError);}
            });
        } catch(const Failure& e){report(e.code);}
    }
    void handle(Event e) {
        std::function<void()> callback;
        std::function<void(SimpleCommKitBlePeripheral)> advertisement;
        std::function<void(std::vector<uint8_t>)> notification;
        std::function<void(ErrorCode)> onError;
        SubscriptionMap discardedSubscriptions;
        {
            std::lock_guard<std::mutex> lock(mutex);
            if(closing || ((e.type==EventType::Connected || e.type==EventType::Disconnected || e.type==EventType::Notification || e.type==EventType::Advertisement) && !generation.accepts(e.generation)))return;
            if(e.type==EventType::Advertisement) {
                if(!scanActive)return;
                const bool exists=discovered.count(e.peripheral.id);
                discovered[e.peripheral.id]=e.peripheral; advertisement=exists?updated:found;
            } else if(e.type==EventType::Notification) {
                auto it=subscriptions.find({e.service,e.characteristic});
                if(it==subscriptions.end() || it->second.revision!=e.subscriptionRevision)return;
                notification=it->second.callback;
            } else if(e.type==EventType::Error) onError=error;
            else {
                if(e.type==EventType::ScanStarted) {if(scanActive)return;scanActive=true;}
                if(e.type==EventType::ScanStopped) {if(!scanActive)return;scanActive=false;}
                if(e.type==EventType::Connected){if(connectionActive)return;connectionActive=true;}
                if(e.type==EventType::Disconnected){
                    if(!connectionActive)return;
                    connectionActive=false;subscriptions.swap(discardedSubscriptions);
                    if(e.generation!=requestedDisconnectGeneration)generation.advance();
                }
                callback=callbacks[e.type];
            }
        }
        if(advertisement) advertisement(e.peripheral.value);
        else if(notification) notification(std::move(e.data));
        else if(onError) onError(e.error);
        else if(callback) callback();
    }
};
std::pair<std::string,std::string> key(const std::string& s,const std::string& c) {return {canonicalUuid(s),canonicalUuid(c)};}
} // namespace
class SimpleCommKitBleCentralPrivate {
public:
    std::shared_ptr<CentralState> state;
    SimpleCommKitBleCentralPrivate():state(std::make_shared<CentralState>(createBackend())){state->initialize();}
    ~SimpleCommKitBleCentralPrivate(){state->shutdown();}
};
SimpleCommKitBleCentral::SimpleCommKitBleCentral():d_ptr(std::make_unique<SimpleCommKitBleCentralPrivate>()){}
SimpleCommKitBleCentral::~SimpleCommKitBleCentral()=default;
bool SimpleCommKitBleCentral::Bluetooth_Enabled(){
    try {auto backend=createBackend();Operation op;bool enabled=backend->enabled(op);backend->shutdown();return enabled;}catch(...){return false;}
}
std::vector<SimpleCommKitBleAdapter> SimpleCommKitBleCentral::get_Adapters(){
    auto s=d_ptr->state;return s->call([](auto& b,const auto& op){std::vector<SimpleCommKitBleAdapter> out;for(auto& a:b.adapters(op))out.push_back(a.value);return out;});
}
std::optional<SimpleCommKitBleAdapter> SimpleCommKitBleCentral::get_CurrentAdapter(){
    auto s=d_ptr->state;std::lock_guard<std::mutex> lock(s->mutex);if(s->adapter)return s->adapter->value;return {};
}
void SimpleCommKitBleCentral::set_CurrentAdapter(const SimpleCommKitBleAdapter& value){
    auto s=d_ptr->state;
    if(s->events.isCurrent()){s->report(ErrorCodes::SimpleCommKitBleReentrantOperation);return;}
    s->generation.advance();
    {std::lock_guard<std::mutex> lock(s->mutex);s->connectionActive=false;}
    bool hadPeripheral;
    {std::lock_guard<std::mutex> lock(s->mutex);hadPeripheral=s->selected.has_value();}
    auto chosen=s->call([value,hadPeripheral](auto& b,const auto& op){
        std::vector<Adapter> matches;
        for(auto& a:b.adapters(op))if((!value.dev_address.empty() && a.value.dev_address==value.dev_address) ||
            (!value.dev_identifier.empty() && (a.id==value.dev_identifier || a.value.dev_identifier==value.dev_identifier)))matches.push_back(a);
        if(matches.size()>1)throw Failure(ErrorCodes::SimpleCommKitBleAmbiguousIdentity);
        if(matches.empty())throw Failure(ErrorCodes::SimpleCommKitBleSetAdapterError);
        if(b.scanning(op))b.stopScan(op);
        if(hadPeripheral && b.connected(op))b.disconnect(op);
        b.selectAdapter(matches.front().id,op);return std::optional<Adapter>(matches.front());
    });
    if(chosen){s->clearSubscriptions();std::lock_guard<std::mutex> lock(s->mutex);s->adapter=std::move(chosen);s->selected.reset();s->discovered.clear();s->scanActive=false;}
}
void SimpleCommKitBleCentral::adapter_Power_On(){auto s=d_ptr->state;s->call([](auto& b,const auto& o){b.setPowered(true,o);});}
void SimpleCommKitBleCentral::adapter_Power_Off(){auto s=d_ptr->state;s->call([](auto& b,const auto& o){b.setPowered(false,o);});}
bool SimpleCommKitBleCentral::adapter_Is_Powered(){auto s=d_ptr->state;return s->call([](auto& b,const auto& o){return b.powered(o);},true);}
#define VOID_CALLBACK(method,event) \
void SimpleCommKitBleCentral::method(std::function<void()> callback){auto s=d_ptr->state;std::lock_guard<std::mutex> lock(s->mutex);s->callbacks[EventType::event].swap(callback);}
VOID_CALLBACK(adapter_Set_Callback_On_Power_On,PowerOn)
VOID_CALLBACK(adapter_Set_Callback_On_Power_Off,PowerOff)
VOID_CALLBACK(adapter_Set_Callback_On_Scan_Start,ScanStarted)
VOID_CALLBACK(adapter_Set_Callback_On_Scan_Stop,ScanStopped)
VOID_CALLBACK(peripheral_Set_Callback_On_Connected,Connected)
VOID_CALLBACK(peripheral_Set_Callback_On_Disconnected,Disconnected)
#undef VOID_CALLBACK
void SimpleCommKitBleCentral::adapter_Set_Callback_On_Scan_Found(std::function<void(SimpleCommKitBlePeripheral)> callback){auto s=d_ptr->state;std::lock_guard<std::mutex> lock(s->mutex);s->found.swap(callback);}
void SimpleCommKitBleCentral::adapter_Set_Callback_On_Scan_Updated(std::function<void(SimpleCommKitBlePeripheral)> callback){auto s=d_ptr->state;std::lock_guard<std::mutex> lock(s->mutex);s->updated.swap(callback);}
void SimpleCommKitBleCentral::set_Callback_Error(std::function<void(ErrorCode)> callback){auto s=d_ptr->state;std::lock_guard<std::mutex> lock(s->mutex);s->error.swap(callback);}
std::vector<SimpleCommKitBlePeripheral> SimpleCommKitBleCentral::adapter_Get_Scan_Results(){
    auto s=d_ptr->state;std::lock_guard<std::mutex> lock(s->mutex);std::vector<SimpleCommKitBlePeripheral> out;for(auto& [id,p]:s->discovered)out.push_back(p.value);return out;
}
#define LIST(method,filter) \
std::vector<SimpleCommKitBlePeripheral> SimpleCommKitBleCentral::method(){auto s=d_ptr->state;return s->call([s](auto& b,const auto& o){std::vector<SimpleCommKitBlePeripheral> out;auto values=b.peripherals(DeviceFilter::filter,o);std::lock_guard<std::mutex> lock(s->mutex);for(auto& p:values){s->discovered[p.id]=p;out.push_back(p.value);}return out;});}
LIST(adapter_Get_Paired_Peripherals,Paired)
LIST(adapter_Get_Connected_Peripherals,Connected)
#undef LIST
void SimpleCommKitBleCentral::adapter_Scan_Start(){auto s=d_ptr->state;s->call([s](auto& b,const auto& o){if(!b.scanning(o)){std::lock_guard<std::mutex> lock(s->mutex);s->discovered.clear();}b.startScan(o);});}
void SimpleCommKitBleCentral::adapter_Scan_Stop(){auto s=d_ptr->state;if(s->events.isCurrent())s->request([](auto& b,const auto& o){b.stopScan(o);});else s->call([](auto& b,const auto& o){b.stopScan(o);});}
void SimpleCommKitBleCentral::adapter_Scan_For(int timeout_ms){
    auto s=d_ptr->state;
    if(timeout_ms<0){s->report(ErrorCodes::SimpleCommKitBleInvalidArgument);return;}
    if(s->events.isCurrent()){s->report(ErrorCodes::SimpleCommKitBleReentrantOperation);return;}
    // Duration waits outside the operation executor so stop/disconnect stay responsive.
    auto started=s->call([s](auto& b,const auto& o){if(!b.scanning(o)){std::lock_guard<std::mutex> lock(s->mutex);s->discovered.clear();}b.startScan(o);return true;});
    if(!started)return;
    const auto end=Clock::now()+std::chrono::milliseconds(timeout_ms);
    while(Clock::now()<end){
        {std::lock_guard<std::mutex> lock(s->mutex);if(s->closing)return;}
        if(!s->call([](auto& b,const auto& o){return b.scanning(o);},true))return;
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
    }
    s->call([](auto& b,const auto& o){b.stopScan(o);});
}
bool SimpleCommKitBleCentral::adapter_Scan_Is_Active(){auto s=d_ptr->state;return s->call([](auto& b,const auto& o){return b.scanning(o);},true);}
bool SimpleCommKitBleCentral::adapter_Initialized(){auto s=d_ptr->state;std::lock_guard<std::mutex> lock(s->mutex);return s->adapter.has_value();}
void SimpleCommKitBleCentral::set_CurrentPeripheral(const SimpleCommKitBlePeripheral& value){
    auto s=d_ptr->state;if(s->events.isCurrent()){s->report(ErrorCodes::SimpleCommKitBleReentrantOperation);return;}
    std::vector<Peripheral> matches;
    {std::lock_guard<std::mutex> lock(s->mutex);for(auto& [id,p]:s->discovered){
        if((!value.address.empty() && p.value.address==value.address && p.value.address_type==value.address_type) ||
           (value.address.empty() && !value.identifier.empty() && p.value.identifier==value.identifier))matches.push_back(p);
    }}
    if(matches.size()!=1){s->report(matches.empty()?ErrorCodes::SimpleCommKitBleCurrentPeripheralSetError:ErrorCodes::SimpleCommKitBleAmbiguousIdentity);return;}
    s->generation.advance();auto p=matches.front();
    {std::lock_guard<std::mutex> lock(s->mutex);s->connectionActive=false;}
    bool hadPeripheral;
    {std::lock_guard<std::mutex> lock(s->mutex);hadPeripheral=s->selected.has_value();}
    bool ok=s->call([p,hadPeripheral](auto& b,const auto& o){if(b.scanning(o))b.stopScan(o);if(hadPeripheral && b.connected(o))b.disconnect(o);b.selectPeripheral(p.id,o);return true;});
    if(ok){s->clearSubscriptions();std::lock_guard<std::mutex> lock(s->mutex);s->selected=p;s->scanActive=false;}
}
#define QUERY(method,backend,type) \
type SimpleCommKitBleCentral::method(){auto s=d_ptr->state;return s->call([](auto& b,const auto& o){return b.backend(o);},true);}
QUERY(peripheral_Get_Tx_Power,txPower,int16_t)
QUERY(peripheral_Get_Mtu,mtu,uint16_t)
QUERY(peripheral_Is_Connected,connected,bool)
QUERY(peripheral_Is_Connectable,connectable,bool)
QUERY(peripheral_Is_Paired,paired,bool)
#undef QUERY
void SimpleCommKitBleCentral::peripheral_Connect(){
    auto s=d_ptr->state;
    if(s->events.isCurrent()){s->report(ErrorCodes::SimpleCommKitBleReentrantOperation);return;}
    s->generation.advance();s->clearSubscriptions();
    s->call([](auto& b,const auto& o){if(b.scanning(o))b.stopScan(o);b.connect(o);});
}
void SimpleCommKitBleCentral::peripheral_Disconnect(){
    auto s=d_ptr->state;const auto disconnectGeneration=s->generation.advance();
    {std::lock_guard<std::mutex> lock(s->mutex);s->requestedDisconnectGeneration=disconnectGeneration;}
    s->clearSubscriptions();
    if(s->events.isCurrent())s->request([](auto& b,const auto& o){b.disconnect(o);});else s->call([](auto& b,const auto& o){b.disconnect(o);});
}
void SimpleCommKitBleCentral::peripheral_Unpair(){auto s=d_ptr->state;s->call([](auto& b,const auto& o){b.unpair(o);});}
std::vector<SimpleCommKitBleService> SimpleCommKitBleCentral::peripheral_Services(){auto s=d_ptr->state;return s->call([](auto& b,const auto& o){return b.services(o);});}
std::vector<uint8_t> SimpleCommKitBleCentral::peripheral_Read(const std::string& service,const std::string& characteristic){return peripheral_Read(service,characteristic,"");}
std::vector<uint8_t> SimpleCommKitBleCentral::peripheral_Read(const std::string& service,const std::string& characteristic,const std::string& descriptor){auto s=d_ptr->state;return s->call([=](auto& b,const auto& o){return b.read(service,characteristic,descriptor,o);});}
void SimpleCommKitBleCentral::peripheral_Write_Request(const std::string& service,const std::string& characteristic,const std::vector<uint8_t>& data){auto s=d_ptr->state;s->call([=](auto& b,const auto& o){b.write(service,characteristic,"",data,WriteType::Request,o);});}
void SimpleCommKitBleCentral::peripheral_Write_Command(const std::string& service,const std::string& characteristic,const std::vector<uint8_t>& data){auto s=d_ptr->state;s->call([=](auto& b,const auto& o){b.write(service,characteristic,"",data,WriteType::Command,o);});}
void SimpleCommKitBleCentral::peripheral_Write(const std::string& service,const std::string& characteristic,const std::string& descriptor,const std::vector<uint8_t>& data){auto s=d_ptr->state;s->call([=](auto& b,const auto& o){b.write(service,characteristic,descriptor,data,WriteType::Request,o);});}
namespace {
void subscribe(const std::shared_ptr<CentralState>& s,const std::string& service,const std::string& characteristic,std::function<void(std::vector<uint8_t>)> callback,SubscriptionType type){
    s->call([=](auto& b,const auto& o){auto k=key(service,characteristic);b.subscribe(service,characteristic,type,o);CentralState::Subscription previous{};
        {std::lock_guard<std::mutex> lock(s->mutex);previous=std::move(s->subscriptions[k]);s->subscriptions[k]={++s->revision,callback};}});
}
}
void SimpleCommKitBleCentral::peripheral_Notify(const std::string& service,const std::string& characteristic,std::function<void(std::vector<uint8_t>)> callback){subscribe(d_ptr->state,service,characteristic,std::move(callback),SubscriptionType::Notify);}
void SimpleCommKitBleCentral::peripheral_Indicate(const std::string& service,const std::string& characteristic,std::function<void(std::vector<uint8_t>)> callback){subscribe(d_ptr->state,service,characteristic,std::move(callback),SubscriptionType::Indicate);}
void SimpleCommKitBleCentral::peripheral_Unsubscribe(const std::string& service,const std::string& characteristic){
    auto s=d_ptr->state;s->call([=](auto& b,const auto& o){auto k=key(service,characteristic);b.unsubscribe(service,characteristic,o);CentralState::SubscriptionMap::node_type removed;
        {std::lock_guard<std::mutex> lock(s->mutex);removed=s->subscriptions.extract(k);}});
    // Drain callbacks already in flight before returning from unsubscribe.
    if(!s->events.isCurrent()) {
        try {waitFor(s->events,s->generation.begin(),[](const Operation&){});}
        catch(const Failure& e){s->report(e.code);}
    }
}
} // namespace SimpleCommKit
