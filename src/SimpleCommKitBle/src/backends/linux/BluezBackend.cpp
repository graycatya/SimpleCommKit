#include "../BleRuntime.h"
#include <dbus/dbus.h>
#include <algorithm>
#include <set>
#include <variant>

namespace SimpleCommKit::Ble {
namespace {
constexpr const char* AdapterIf = "org.bluez.Adapter1";
constexpr const char* DeviceIf = "org.bluez.Device1";
constexpr const char* ServiceIf = "org.bluez.GattService1";
constexpr const char* CharIf = "org.bluez.GattCharacteristic1";
constexpr const char* DescIf = "org.bluez.GattDescriptor1";
using Message = std::unique_ptr<DBusMessage, decltype(&dbus_message_unref)>;
using Scalar = std::variant<std::monostate, std::string, bool, int16_t, uint16_t,
                            std::vector<uint8_t>, std::vector<std::string>,
                            std::map<uint16_t, std::vector<uint8_t>>,
                            std::map<std::string, std::vector<uint8_t>>>;
using Props = std::map<std::string, Scalar>;
using Objects = std::map<std::string, std::map<std::string, Props>>;
ErrorCode mapError(const std::string& name) {
    if (name.find("NotAuthorized") != std::string::npos || name.find("AccessDenied") != std::string::npos ||
        name.find("NotPermitted") != std::string::npos) return ErrorCodes::SimpleCommKitBlePermissionDenied;
    if (name.find("NotSupported") != std::string::npos || name.find("UnknownMethod") != std::string::npos)
        return ErrorCodes::SimpleCommKitBleNotSupported;
    if (name.find("Invalid") != std::string::npos) return ErrorCodes::SimpleCommKitBleInvalidArgument;
    if (name.find("NoReply") != std::string::npos || name.find("Timeout") != std::string::npos)
        return ErrorCodes::SimpleCommKitBleTimeout;
    if (name.find("ServiceUnknown") != std::string::npos || name.find("Disconnected") != std::string::npos ||
        name.find("NameHasNoOwner") != std::string::npos) return ErrorCodes::SimpleCommKitBleBackendUnavailable;
    return ErrorCodes::SimpleCommKitBleGattError;
}
Scalar parse(DBusMessageIter it) {
    const int type = dbus_message_iter_get_arg_type(&it);
    if (type == DBUS_TYPE_VARIANT) { DBusMessageIter child; dbus_message_iter_recurse(&it, &child); return parse(child); }
    if (type == DBUS_TYPE_STRING || type == DBUS_TYPE_OBJECT_PATH) {
        const char* s; dbus_message_iter_get_basic(&it, &s); return std::string(s);
    }
    if (type == DBUS_TYPE_BOOLEAN) { dbus_bool_t b; dbus_message_iter_get_basic(&it, &b); return bool(b); }
    if (type == DBUS_TYPE_INT16) { int16_t v; dbus_message_iter_get_basic(&it, &v); return v; }
    if (type == DBUS_TYPE_UINT16) { uint16_t v; dbus_message_iter_get_basic(&it, &v); return v; }
    if (type == DBUS_TYPE_ARRAY) {
        DBusMessageIter child; dbus_message_iter_recurse(&it, &child);
        const int element = dbus_message_iter_get_element_type(&it);
        if (element == DBUS_TYPE_BYTE) {
            std::vector<uint8_t> bytes;
            while (dbus_message_iter_get_arg_type(&child) == DBUS_TYPE_BYTE) {
                uint8_t v; dbus_message_iter_get_basic(&child, &v); bytes.push_back(v); dbus_message_iter_next(&child);
            }
            return bytes;
        }
        if (element == DBUS_TYPE_STRING) {
            std::vector<std::string> values;
            while (dbus_message_iter_get_arg_type(&child) == DBUS_TYPE_STRING) {
                values.push_back(std::get<std::string>(parse(child))); dbus_message_iter_next(&child);
            }
            return values;
        }
        if (element == DBUS_TYPE_DICT_ENTRY) {
            std::map<uint16_t, std::vector<uint8_t>> manufacturer;
            while (dbus_message_iter_get_arg_type(&child) == DBUS_TYPE_DICT_ENTRY) {
                DBusMessageIter entry; dbus_message_iter_recurse(&child, &entry);
                if (dbus_message_iter_get_arg_type(&entry) == DBUS_TYPE_STRING) {
                    std::map<std::string, std::vector<uint8_t>> values;
                    while(dbus_message_iter_get_arg_type(&child)==DBUS_TYPE_DICT_ENTRY) {
                        DBusMessageIter item;dbus_message_iter_recurse(&child,&item);
                        auto key=std::get<std::string>(parse(item));dbus_message_iter_next(&item);
                        auto value=parse(item);
                        if(auto bytes=std::get_if<std::vector<uint8_t>>(&value))values[key]=*bytes;
                        dbus_message_iter_next(&child);
                    }
                    return values;
                }
                if (dbus_message_iter_get_arg_type(&entry) != DBUS_TYPE_UINT16) return {};
                uint16_t key; dbus_message_iter_get_basic(&entry, &key); dbus_message_iter_next(&entry);
                auto value = parse(entry);
                if (auto bytes = std::get_if<std::vector<uint8_t>>(&value)) manufacturer[key] = *bytes;
                dbus_message_iter_next(&child);
            }
            return manufacturer;
        }
    }
    return {};
}
Props parseProps(DBusMessageIter array) {
    Props props;
    DBusMessageIter entries; dbus_message_iter_recurse(&array, &entries);
    while (dbus_message_iter_get_arg_type(&entries) == DBUS_TYPE_DICT_ENTRY) {
        DBusMessageIter entry; dbus_message_iter_recurse(&entries, &entry);
        auto key = std::get<std::string>(parse(entry)); dbus_message_iter_next(&entry);
        props[key] = parse(entry); dbus_message_iter_next(&entries);
    }
    return props;
}
template<class T> T get(const Props& props, const std::string& key, T fallback = {}) {
    auto it = props.find(key);
    if (it != props.end()) if (auto p = std::get_if<T>(&it->second)) return *p;
    return fallback;
}
void basic(DBusMessageIter& it, int type, const char* value) { dbus_message_iter_append_basic(&it, type, &value); }
void option(DBusMessageIter& array, const char* key, const char* value) {
    DBusMessageIter entry, variant;
    dbus_message_iter_open_container(&array, DBUS_TYPE_DICT_ENTRY, nullptr, &entry);
    basic(entry, DBUS_TYPE_STRING, key);
    dbus_message_iter_open_container(&entry, DBUS_TYPE_VARIANT, "s", &variant);
    basic(variant, DBUS_TYPE_STRING, value);
    dbus_message_iter_close_container(&entry, &variant); dbus_message_iter_close_container(&array, &entry);
}
class Bus {
public:
    Bus() {
        static const bool initialized = dbus_threads_init_default();
        if (!initialized) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
        DBusError error; dbus_error_init(&error);
        connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &error);
        if (!connection) { std::string message = error.message ? error.message : "System bus unavailable";
            dbus_error_free(&error); throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable, message); }
        dbus_connection_set_exit_on_disconnect(connection, false);
    }
    ~Bus() { dbus_connection_close(connection); dbus_connection_unref(connection); }
    Message call(const std::string& path, const char* iface, const char* method, const Operation& op,
                 std::function<void(DBusMessageIter&)> args = {}) {
        op.check();
        Message request(dbus_message_new_method_call("org.bluez", path.c_str(), iface, method), dbus_message_unref);
        if (!request) throw std::bad_alloc();
        if (args) { DBusMessageIter it; dbus_message_iter_init_append(request.get(), &it); args(it); }
        DBusPendingCall* raw = nullptr;
        if (!dbus_connection_send_with_reply(connection, request.get(), &raw, 30000) || !raw) throw std::bad_alloc();
        std::unique_ptr<DBusPendingCall, decltype(&dbus_pending_call_unref)> pending(raw, dbus_pending_call_unref);
        while (!dbus_pending_call_get_completed(raw)) {
            try { op.check(); } catch (...) { dbus_pending_call_cancel(raw); throw; }
            if (!dbus_connection_read_write_dispatch(connection, 5)) {
                dbus_pending_call_cancel(raw); throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
            }
        }
        op.check();
        Message reply(dbus_pending_call_steal_reply(raw), dbus_message_unref);
        if (!reply) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
        if (dbus_message_get_type(reply.get()) == DBUS_MESSAGE_TYPE_ERROR) {
            const char* name = dbus_message_get_error_name(reply.get());
            throw Failure(mapError(name ? name : ""), name ? name : "D-Bus error");
        }
        return reply;
    }
    Objects objects(const Operation& op) {
        auto reply = call("/", "org.freedesktop.DBus.ObjectManager", "GetManagedObjects", op);
        DBusMessageIter root, paths; dbus_message_iter_init(reply.get(), &root); dbus_message_iter_recurse(&root, &paths);
        Objects objects;
        while (dbus_message_iter_get_arg_type(&paths) == DBUS_TYPE_DICT_ENTRY) {
            DBusMessageIter path, interfaces; dbus_message_iter_recurse(&paths, &path);
            auto key = std::get<std::string>(parse(path)); dbus_message_iter_next(&path); dbus_message_iter_recurse(&path, &interfaces);
            while (dbus_message_iter_get_arg_type(&interfaces) == DBUS_TYPE_DICT_ENTRY) {
                DBusMessageIter entry; dbus_message_iter_recurse(&interfaces, &entry);
                auto iface = std::get<std::string>(parse(entry)); dbus_message_iter_next(&entry);
                objects[key][iface] = parseProps(entry); dbus_message_iter_next(&interfaces);
            }
            dbus_message_iter_next(&paths);
        }
        return objects;
    }
    DBusConnection* connection = nullptr;
};
Peripheral peripheral(const std::string& id, const Props& props) {
    Peripheral p; p.id = id; p.name = get<std::string>(props, "Name");
    p.value.identifier = p.name; p.value.address = get<std::string>(props, "Address");
    const auto type = get<std::string>(props, "AddressType");
    p.value.address_type = type == "public" ? PUBLIC : type == "random" ? RANDOM : UNSPECIFIED;
    p.value.rssi = get<int16_t>(props, "RSSI");
    p.value.manufacturer = get<std::map<uint16_t, std::vector<uint8_t>>>(props, "ManufacturerData");
    return p;
}
class BluezBackend final : public IBleCentralBackend {
public:
    ~BluezBackend() override { shutdown(); }
    Capabilities capabilities() const override { return {true,true,true,true,false,true,true}; }
    void setEventSink(EventSink sink) override { std::lock_guard<std::mutex> lock(mutex_); sink_ = std::move(sink); }
    void shutdown() noexcept override {
        stopping_->store(true);
        { std::lock_guard<std::mutex> lock(mutex_); sink_ = {}; }
        if (monitor_.joinable()) monitor_.join();
        if(bus_ && !ownedDevice_.empty()) {
            try {Operation cleanup;cleanup.deadline=Clock::now()+std::chrono::milliseconds(500);
                bus_->call(ownedDevice_,DeviceIf,"Disconnect",cleanup);}catch(...){}
            ownedDevice_.clear();
        }
        // Closing the private command connection releases discovery/notify sessions.
        bus_.reset();
    }
    bool enabled(const Operation& op) override {
        for (auto& [id, interfaces] : bus().objects(op)) {
            auto it = interfaces.find(AdapterIf);
            if (it != interfaces.end() && get<bool>(it->second, "Powered")) return true;
        }
        return false;
    }
    std::vector<Adapter> adapters(const Operation& op) override {
        std::vector<Adapter> result;
        for (auto& [id, interfaces] : bus().objects(op)) if (interfaces.count(AdapterIf)) {
            const auto& props = interfaces.at(AdapterIf);
            result.push_back({id,{id,get<std::string>(props,"Address")}});
        }
        startMonitor(); return result;
    }
    void selectAdapter(const std::string& id, const Operation& op) override {
        auto objects = bus().objects(op);
        if (!objects.count(id) || !objects.at(id).count(AdapterIf)) throw Failure(ErrorCodes::SimpleCommKitBleSetAdapterError);
        std::lock_guard<std::mutex> lock(mutex_); adapter_ = id; device_.clear(); generation_ = op.generation; scanning_ = false; subscriptions_.clear();
    }
    bool powered(const Operation& op) override { return get<bool>(properties(adapter(), AdapterIf, op),"Powered"); }
    void setPowered(bool on, const Operation& op) override {
        bus().call(adapter(), "org.freedesktop.DBus.Properties", "Set", op, [on](auto& it) {
            basic(it,DBUS_TYPE_STRING,AdapterIf); basic(it,DBUS_TYPE_STRING,"Powered");
            DBusMessageIter variant; dbus_message_iter_open_container(&it,DBUS_TYPE_VARIANT,"b",&variant);
            dbus_bool_t value = on; dbus_message_iter_append_basic(&variant,DBUS_TYPE_BOOLEAN,&value);
            dbus_message_iter_close_container(&it,&variant);
        });
    }
    void startScan(const Operation& op) override {
        if (scanning(op)) return;
        bus().call(adapter(),AdapterIf,"SetDiscoveryFilter",op,[](auto& it) {
            DBusMessageIter dict; dbus_message_iter_open_container(&it,DBUS_TYPE_ARRAY,"{sv}",&dict);
            option(dict,"Transport","le"); dbus_message_iter_close_container(&it,&dict);
        });
        bus().call(adapter(),AdapterIf,"StartDiscovery",op);
        { std::lock_guard<std::mutex> lock(mutex_); scanning_=true; scanGeneration_=op.generation; ++scanSession_; }
        emit(Event{EventType::ScanStarted,op.generation});
    }
    void stopScan(const Operation& op) override {
        if (!scanning(op)) return;
        bus().call(adapter(),AdapterIf,"StopDiscovery",op);
        { std::lock_guard<std::mutex> lock(mutex_); scanning_=false; }
        emit(Event{EventType::ScanStopped,op.generation});
    }
    bool scanning(const Operation&) override { std::lock_guard<std::mutex> lock(mutex_); return scanning_; }
    std::vector<Peripheral> peripherals(DeviceFilter filter, const Operation& op) override {
        std::vector<Peripheral> result; auto selected=adapter();
        for (auto& [id,interfaces] : bus().objects(op)) if (interfaces.count(DeviceIf)) {
            auto& props=interfaces.at(DeviceIf);
            if (get<std::string>(props,"Adapter") != selected) continue;
            if (filter==DeviceFilter::Paired && !get<bool>(props,"Paired")) continue;
            if (filter==DeviceFilter::Connected && !get<bool>(props,"Connected")) continue;
            result.push_back(peripheral(id,props));
        }
        return result;
    }
    void selectPeripheral(const std::string& id,const Operation& op) override {
        op.check();
        // Selection is a logical handle from the facade's scan cache. BlueZ may
        // remove an unconnected Device1 after discovery stops; resolve it again
        // during connect instead of treating that as a missing system backend.
        const auto owner=adapter();
        if(id.compare(0,owner.size()+5,owner+"/dev_")!=0)
            throw Failure(ErrorCodes::SimpleCommKitBleCurrentPeripheralSetError);
        std::lock_guard<std::mutex> lock(mutex_); device_=id; generation_=op.generation; disconnecting_=false; subscriptions_.clear();
    }
    void connect(const Operation& op) override {
        {std::lock_guard<std::mutex> lock(mutex_); generation_=op.generation;disconnecting_=false;}
        const auto path=device();
        try {
            const auto props=resolveDevice(path,op);
            if(!get<bool>(props,"Connected"))ownedDevice_=path;
            bus().call(path,DeviceIf,"Connect",op);
            while (!get<bool>(properties(path,DeviceIf,op),"ServicesResolved")) { op.check(); std::this_thread::sleep_for(std::chrono::milliseconds(5)); }
            stopRecoveryDiscovery();
        } catch(...) {
            if(ownedDevice_==path) {
                try {Operation cleanup;cleanup.deadline=Clock::now()+std::chrono::milliseconds(500);
                    bus().call(path,DeviceIf,"Disconnect",cleanup);}catch(...){}
                ownedDevice_.clear();
            }
            stopRecoveryDiscovery();
            throw;
        }
    }
    void disconnect(const Operation& op) override {
        {std::lock_guard<std::mutex> lock(mutex_); generation_=op.generation;disconnecting_=true;}
        try {bus().call(device(),DeviceIf,"Disconnect",op);}
        catch(...) {std::lock_guard<std::mutex> lock(mutex_);disconnecting_=false;throw;}
        ownedDevice_.clear();
        emit(Event{EventType::Disconnected,op.generation});
    }
    bool connected(const Operation& op) override { return get<bool>(properties(device(),DeviceIf,op),"Connected"); }
    bool connectable(const Operation&) override { throw Failure(ErrorCodes::SimpleCommKitBleNotSupported); }
    bool paired(const Operation& op) override { return get<bool>(properties(device(),DeviceIf,op),"Paired"); }
    void unpair(const Operation& op) override {
        const auto id=device(); bus().call(adapter(),AdapterIf,"RemoveDevice",op,[&](auto& it){basic(it,DBUS_TYPE_OBJECT_PATH,id.c_str());});
    }
    int16_t txPower(const Operation& op) override {
        auto props=properties(device(),DeviceIf,op);
        if (!props.count("TxPower")) throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
        return get<int16_t>(props,"TxPower");
    }
    uint16_t mtu(const Operation&) override { throw Failure(ErrorCodes::SimpleCommKitBleNotSupported); }
    std::vector<SimpleCommKitBleService> services(const Operation& op) override {
        requireConnected(op); const auto id=device(); auto objects=bus().objects(op);
        std::vector<SimpleCommKitBleService> result;
        for(auto& [path,interfaces]:objects) if(interfaces.count(ServiceIf) && get<std::string>(interfaces.at(ServiceIf),"Device")==id) {
            SimpleCommKitBleService service{}; service.uuid=canonicalUuid(get<std::string>(interfaces.at(ServiceIf),"UUID"));
            if(objects.count(id) && objects.at(id).count(DeviceIf)) {
                const auto data=get<std::map<std::string,std::vector<uint8_t>>>(objects.at(id).at(DeviceIf),"ServiceData");
                for(const auto& [uuid,bytes]:data)if(canonicalUuid(uuid)==service.uuid)service.data=bytes;
            }
            for(auto& [cp,ci]:objects) if(ci.count(CharIf) && get<std::string>(ci.at(CharIf),"Service")==path) {
                SimpleCommKitBleCharacteristic c{}; auto& props=ci.at(CharIf); c.uuid=canonicalUuid(get<std::string>(props,"UUID"));
                c.capabilities=get<std::vector<std::string>>(props,"Flags");
                auto has=[&](const char* flag){return std::find(c.capabilities.begin(),c.capabilities.end(),flag)!=c.capabilities.end();};
                c.can_read=has("read"); c.can_write_request=has("write"); c.can_write_command=has("write-without-response");
                c.can_notify=has("notify"); c.can_indicate=has("indicate");
                for(auto& [dp,di]:objects) if(di.count(DescIf) && get<std::string>(di.at(DescIf),"Characteristic")==cp)
                    c.descriptors_uuid.push_back(canonicalUuid(get<std::string>(di.at(DescIf),"UUID")));
                service.characteristics.push_back(std::move(c));
            }
            result.push_back(std::move(service));
        }
        return result;
    }
    std::vector<uint8_t> read(const std::string& s,const std::string& c,const std::string& d,const Operation& op) override {
        auto path=resolve(s,c,d,op);
        auto reply=bus().call(path,d.empty()?CharIf:DescIf,"ReadValue",op,[](auto& it){
            DBusMessageIter dict; dbus_message_iter_open_container(&it,DBUS_TYPE_ARRAY,"{sv}",&dict); dbus_message_iter_close_container(&it,&dict);
        });
        DBusMessageIter it; dbus_message_iter_init(reply.get(),&it); return std::get<std::vector<uint8_t>>(parse(it));
    }
    void write(const std::string& s,const std::string& c,const std::string& d,const std::vector<uint8_t>& data,WriteType type,const Operation& op) override {
        auto path=resolve(s,c,d,op);
        bus().call(path,d.empty()?CharIf:DescIf,"WriteValue",op,[&](auto& it){
            DBusMessageIter bytes,dict; dbus_message_iter_open_container(&it,DBUS_TYPE_ARRAY,"y",&bytes);
            for(auto byte:data) dbus_message_iter_append_basic(&bytes,DBUS_TYPE_BYTE,&byte);
            dbus_message_iter_close_container(&it,&bytes); dbus_message_iter_open_container(&it,DBUS_TYPE_ARRAY,"{sv}",&dict);
            if(d.empty()) option(dict,"type",type==WriteType::Request?"request":"command");
            dbus_message_iter_close_container(&it,&dict);
        });
    }
    void subscribe(const std::string& s,const std::string& c,SubscriptionType type,const Operation& op) override {
        auto path=resolve(s,c,"",op); auto props=properties(path,CharIf,op);
        auto flags=get<std::vector<std::string>>(props,"Flags");
        const char* required=type==SubscriptionType::Notify?"notify":"indicate";
        if(std::find(flags.begin(),flags.end(),required)==flags.end()) throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
        // StartNotify does not let clients select notify vs indicate on a dual-capability characteristic.
        if(std::find(flags.begin(),flags.end(),"notify")!=flags.end() && std::find(flags.begin(),flags.end(),"indicate")!=flags.end())
            throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
        bus().call(path,CharIf,"StartNotify",op);
        std::lock_guard<std::mutex> lock(mutex_); subscriptions_[path]={canonicalUuid(s),canonicalUuid(c)};
    }
    void unsubscribe(const std::string& s,const std::string& c,const Operation& op) override {
        auto path=resolve(s,c,"",op); bus().call(path,CharIf,"StopNotify",op);
        std::lock_guard<std::mutex> lock(mutex_); subscriptions_.erase(path);
    }
private:
    Bus& bus() { if(!bus_) bus_=std::make_unique<Bus>(); return *bus_; }
    std::string adapter() { std::lock_guard<std::mutex> lock(mutex_); if(adapter_.empty()) throw Failure(ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet); return adapter_; }
    std::string device() { std::lock_guard<std::mutex> lock(mutex_); if(device_.empty()) throw Failure(ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet); return device_; }
    Props properties(const std::string& path,const char* iface,const Operation& op) {
        auto objects=bus().objects(op);
        if(!objects.count(path) || !objects.at(path).count(iface)) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
        return objects.at(path).at(iface);
    }
    Props resolveDevice(const std::string& path,const Operation& op) {
        auto objects=bus().objects(op);
        auto present=[&]{return objects.count(path) && objects.at(path).count(DeviceIf);};
        if(present())return objects.at(path).at(DeviceIf);
        if(!scanning(op) && !recoveryDiscovery_) {
            bus().call(adapter(),AdapterIf,"SetDiscoveryFilter",op,[](auto& it){
                DBusMessageIter dict;dbus_message_iter_open_container(&it,DBUS_TYPE_ARRAY,"{sv}",&dict);
                option(dict,"Transport","le");dbus_message_iter_close_container(&it,&dict);
            });
            bus().call(adapter(),AdapterIf,"StartDiscovery",op);
            recoveryDiscovery_=true;
        }
        // Keep this private discovery session alive through Connect. Stopping it
        // as soon as Device1 reappears can make BlueZ remove the object again.
        while(!present()) {
            op.check();std::this_thread::sleep_for(std::chrono::milliseconds(20));
            objects=bus().objects(op);
        }
        return objects.at(path).at(DeviceIf);
    }
    void stopRecoveryDiscovery() noexcept {
        if(!recoveryDiscovery_)return;
        try {
            Operation cleanup;cleanup.deadline=Clock::now()+std::chrono::milliseconds(500);
            bus().call(adapter(),AdapterIf,"StopDiscovery",cleanup);
            recoveryDiscovery_=false;
        } catch(...) {} // Keep the original operation's failure; shutdown closes the session.
    }
    void requireConnected(const Operation& op) { if(!connected(op)) throw Failure(ErrorCodes::SimpleCommKitBleGattError,"Peripheral disconnected"); }
    std::string resolve(const std::string& s,const std::string& c,const std::string& d,const Operation& op) {
        requireConnected(op); auto objects=bus().objects(op);
        auto find=[&](const char* iface,const char* parent,const std::string& owner,const std::string& uuid){
            std::vector<std::string> matches; const auto key=canonicalUuid(uuid);
            for(auto& [path,interfaces]:objects) if(interfaces.count(iface)) {
                auto& props=interfaces.at(iface);
                if(get<std::string>(props,parent)==owner && canonicalUuid(get<std::string>(props,"UUID"))==key) matches.push_back(path);
            }
            if(matches.size()>1) throw Failure(ErrorCodes::SimpleCommKitBleAmbiguousIdentity);
            if(matches.empty()) throw Failure(ErrorCodes::SimpleCommKitBleGattError,"GATT attribute not found");
            return matches.front();
        };
        auto service=find(ServiceIf,"Device",device(),s); auto characteristic=find(CharIf,"Service",service,c);
        return d.empty()?characteristic:find(DescIf,"Characteristic",characteristic,d);
    }
    void emit(Event event) { EventSink sink; {std::lock_guard<std::mutex> lock(mutex_); sink=sink_;} if(sink) sink(std::move(event)); }
    void startMonitor() {
        if(monitor_.joinable()) return;
        monitor_=std::thread([this]{
            try {
                Bus monitorBus;
                // Signals preserve every notification, including repeated values.
                DBusError error; dbus_error_init(&error);
                dbus_bus_add_match(monitorBus.connection,"type='signal',sender='org.bluez'",&error);
                if(dbus_error_is_set(&error)){dbus_error_free(&error);throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);}
                Objects old;
                uint64_t previousScanSession=0, previousConnectionGeneration=0;
                std::string previousDevice;
                while(!stopping_->load()) {
                    dbus_connection_read_write(monitorBus.connection,50);
                    std::set<std::string> changedDevices;
                    while(auto raw=dbus_connection_pop_message(monitorBus.connection)) {
                        Message message(raw,dbus_message_unref);
                        if(!dbus_message_is_signal(raw,"org.freedesktop.DBus.Properties","PropertiesChanged")) continue;
                        const char* path=dbus_message_get_path(raw); DBusMessageIter it; dbus_message_iter_init(raw,&it);
                        auto iface=std::get<std::string>(parse(it)); dbus_message_iter_next(&it); auto changed=parseProps(it);
                        if(iface==DeviceIf && path)changedDevices.insert(path);
                        std::pair<std::string,std::string> sub; uint64_t gen;
                        { std::lock_guard<std::mutex> lock(mutex_); gen=generation_; if(path && subscriptions_.count(path)) sub=subscriptions_.at(path); }
                        if(iface==CharIf && !sub.first.empty() && changed.count("Value")) {
                            Event event{EventType::Notification,gen}; event.service=sub.first; event.characteristic=sub.second;
                            event.data=get<std::vector<uint8_t>>(changed,"Value"); emit(std::move(event));
                        }
                    }
                    std::string a,d; uint64_t gen,sg,session; bool scanning;
                    {std::lock_guard<std::mutex> lock(mutex_);a=adapter_;d=device_;gen=generation_;sg=scanGeneration_;scanning=scanning_;session=scanSession_;}
                    Operation op; op.cancelled=stopping_; auto now=monitorBus.objects(op);
                    if(!a.empty()) {
                        auto poweredAt=[&](const Objects& o){auto p=o.find(a);return p!=o.end() && p->second.count(AdapterIf) && get<bool>(p->second.at(AdapterIf),"Powered");};
                        if(poweredAt(old)!=poweredAt(now)) emit(Event{poweredAt(now)?EventType::PowerOn:EventType::PowerOff,gen});
                        if(scanning && (!poweredAt(now) || !now.count(a) || !now.at(a).count(AdapterIf) || !get<bool>(now.at(a).at(AdapterIf),"Discovering"))) {
                            bool ended=false;
                            {std::lock_guard<std::mutex> lock(mutex_);if(scanning_ && scanSession_==session){scanning_=false;ended=true;}}
                            if(ended)emit(Event{EventType::ScanStopped,sg});
                            scanning=false;
                        }
                        if(scanning) for(auto& [id,interfaces]:now) if(interfaces.count(DeviceIf) && get<std::string>(interfaces.at(DeviceIf),"Adapter")==a) {
                            if(session!=previousScanSession || changedDevices.count(id) || !old.count(id) || old.at(id)!=interfaces) {Event e{EventType::Advertisement,sg};e.peripheral=peripheral(id,interfaces.at(DeviceIf));emit(std::move(e));}
                        }
                    }
                    if(!d.empty()) {
                        auto connectedAt=[&](const Objects& o){auto p=o.find(d);return p!=o.end() && p->second.count(DeviceIf) && get<bool>(p->second.at(DeviceIf),"Connected");};
                        const bool wasConnected=previousDevice==d && previousConnectionGeneration==gen && connectedAt(old);
                        bool staleOrDisconnecting;
                        {std::lock_guard<std::mutex> lock(mutex_);staleOrDisconnecting=gen!=generation_ || d!=device_ || (connectedAt(now) && disconnecting_);}
                        if(!staleOrDisconnecting && wasConnected!=connectedAt(now)) {
                            if(!connectedAt(now)){std::lock_guard<std::mutex> lock(mutex_);subscriptions_.clear();}
                            emit(Event{connectedAt(now)?EventType::Connected:EventType::Disconnected,gen});
                        }
                    }
                    if(scanning)previousScanSession=session;
                    previousConnectionGeneration=gen;previousDevice=d;
                    old=std::move(now);
                }
            } catch(const Failure& failure) {if(!stopping_->load()){Event e{EventType::Error};e.error=failure.code;emit(std::move(e));}}
              catch(...) {if(!stopping_->load()){Event e{EventType::Error};e.error=ErrorCodes::SimpleCommKitBleBackendUnavailable;emit(std::move(e));}}
        });
    }
    std::unique_ptr<Bus> bus_;
    std::string ownedDevice_;
    bool recoveryDiscovery_=false;
    std::mutex mutex_;
    EventSink sink_;
    std::string adapter_,device_;
    bool scanning_=false,disconnecting_=false;
    uint64_t generation_=0,scanGeneration_=0,scanSession_=0;
    std::map<std::string,std::pair<std::string,std::string>> subscriptions_;
    std::shared_ptr<std::atomic<bool>> stopping_=std::make_shared<std::atomic<bool>>(false);
    std::thread monitor_;
};
} // namespace
std::unique_ptr<IBleCentralBackend> createBackend() { return std::make_unique<BluezBackend>(); }
} // namespace SimpleCommKit::Ble
