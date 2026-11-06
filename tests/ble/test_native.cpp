#include "FakeBackend.h"
#include <iostream>
#include <stdexcept>
using namespace SimpleCommKit;
using namespace SimpleCommKit::Ble;
namespace SimpleCommKit::Ble {std::unique_ptr<IBleCentralBackend> createBackend(){return std::make_unique<FakeBackend>();}}
#define CHECK(test) do{if(!(test))throw std::runtime_error("Check failed: " #test);}while(false)
template<class P> void until(P predicate){auto end=Clock::now()+std::chrono::seconds(3);while(!predicate()){CHECK(Clock::now()<end);std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
template<class F> void fails(ErrorCode expected,F f){try{f();}catch(const Failure& e){CHECK(e.code==expected);return;}throw std::runtime_error("Expected failure");}
void runtime(){
    static_assert(ErrorCodes::SimpleCommKitBleCurrentPeripheralUnSubScribeError==0x1000e);
    static_assert(ErrorCodes::SimpleCommKitBleNotSupported==0x1000f);
    for(ErrorCode c=0x1000f;c<=0x10017;++c)CHECK(SimpleCommKitErrorMap::GetErrorDescription(c)!="Unknown error");
    CHECK(canonicalUuid("180D")=="0000180d-0000-1000-8000-00805f9b34fb");
    CHECK(canonicalUuid("12345678")=="12345678-0000-1000-8000-00805f9b34fb");
    fails(ErrorCodes::SimpleCommKitBleInvalidArgument,[]{canonicalUuid("bad-uuid");});
    Generation domain;auto first=domain.begin();domain.advance();CHECK(!domain.accepts(first.generation));
    fails(ErrorCodes::SimpleCommKitBleCancelled,[&]{first.check();});
    SerialExecutor executor;auto shortOp=domain.begin(std::chrono::milliseconds(15));
    fails(ErrorCodes::SimpleCommKitBleTimeout,[&]{waitFor(executor,shortOp,[](const Operation& o){while(true){o.check();std::this_thread::sleep_for(std::chrono::milliseconds(1));}});});
    std::promise<void> reentered;executor.post([&]{fails(ErrorCodes::SimpleCommKitBleReentrantOperation,[&]{waitFor(executor,domain.begin(),[](const Operation&){return 1;});});reentered.set_value();});
    CHECK(reentered.get_future().wait_for(std::chrono::seconds(2))==std::future_status::ready);
    domain.close();fails(ErrorCodes::SimpleCommKitBleCancelled,[&]{domain.begin();});
}
Peripheral device(){Peripheral p;p.id="native-device";p.name="same-name";p.value.identifier="opaque-uuid";p.value.address_type=UNSPECIFIED;return p;}
void facade(){
    SimpleCommKitBleCentral central;auto control=fake;
    std::atomic<int> found{0},updated{0},start{0},stop{0},notifications{0};std::atomic<ErrorCode> error{0};
    central.set_Callback_Error([&](ErrorCode e){error=e;});
    central.set_CurrentAdapter(central.get_Adapters().front());
    central.adapter_Set_Callback_On_Scan_Start([&]{++start;});
    central.adapter_Set_Callback_On_Scan_Stop([&]{++stop;});
    central.adapter_Set_Callback_On_Scan_Found([&](auto){++found;});
    central.adapter_Set_Callback_On_Scan_Updated([&](auto){++updated;});
    central.adapter_Scan_Start();until([&]{return start==1;});
    auto p=device();control->advertise(p);p.value.rssi=-50;control->advertise(p);
    until([&]{return updated==1;});CHECK(found==1);CHECK(central.adapter_Get_Scan_Results().size()==1);
    central.adapter_Scan_Stop();central.adapter_Scan_Stop();until([&]{return stop==1;});CHECK(!central.adapter_Scan_Is_Active());
    CHECK(central.adapter_Get_Scan_Results().front().rssi==-50);
    central.adapter_Scan_For(-1);until([&]{return error==ErrorCodes::SimpleCommKitBleInvalidArgument;});
    central.set_CurrentPeripheral(p.value);CHECK(central.peripheral_Is_Connectable());central.peripheral_Connect();
    CHECK(central.peripheral_Is_Connected());CHECK(central.peripheral_Get_Tx_Power()==-7);CHECK(central.peripheral_Get_Mtu()==247);
    CHECK(central.peripheral_Read("180d","2a37")==std::vector<uint8_t>({0,1,255}));
    central.peripheral_Notify("180d","2a37",[&](auto data){CHECK(data==std::vector<uint8_t>({255,0}));++notifications;});
    Event notification{EventType::Notification,control->generation.load()};notification.service=canonicalUuid("180d");notification.characteristic=canonicalUuid("2a37");notification.data={255,0};
    control->emit(notification);until([&]{return notifications==1;});
    central.peripheral_Unsubscribe("180d","2a37");control->emit(notification);
    central.adapter_Power_On();until([&]{return error==ErrorCodes::SimpleCommKitBlePermissionDenied;});CHECK(notifications==1);
    central.peripheral_Unpair();until([&]{return error==ErrorCodes::SimpleCommKitBleNotSupported;});
    // Reconnecting must reject an old connection's queued notification.
    central.peripheral_Disconnect();central.peripheral_Connect();
    central.peripheral_Notify("180d","2a37",[&](auto){++notifications;});
    control->emit(notification);
    central.adapter_Power_On();until([&]{return error==ErrorCodes::SimpleCommKitBlePermissionDenied;});
    CHECK(notifications==1);
    central.adapter_Scan_For(0);until([&]{return stop==2;});CHECK(start==2);
}
void platformAddressSelection(){
    SimpleCommKitBleCentral central;auto control=fake;
    std::atomic<int> found{0};std::atomic<ErrorCode> error{0};
    central.set_Callback_Error([&](ErrorCode code){error=code;});
    central.set_CurrentAdapter(central.get_Adapters().front());
    central.adapter_Set_Callback_On_Scan_Found([&](auto){++found;});
    central.adapter_Scan_Start();until([&]{return central.adapter_Scan_Is_Active();});
    auto a=device();a.value.identifier="same-name";a.value.address="01234567-89AB-CDEF-0123-456789ABCDEF";
    auto b=a;b.id="second-device";b.value.address="11234567-89AB-CDEF-0123-456789ABCDEF";
    auto unnamed=a;unnamed.id="unnamed-device";unnamed.value.identifier="";unnamed.value.address="21234567-89AB-CDEF-0123-456789ABCDEF";
    control->advertise(a);control->advertise(b);control->advertise(unnamed);until([&]{return found==3;});
    auto nameOnly=a.value;nameOnly.address.clear();central.set_CurrentPeripheral(nameOnly);
    until([&]{return error==ErrorCodes::SimpleCommKitBleAmbiguousIdentity;});error=0;
    central.set_CurrentPeripheral(b.value);{std::lock_guard<std::mutex> lock(control->mutex);CHECK(control->selectedId==b.id);}central.peripheral_Connect();CHECK(central.peripheral_Is_Connected());CHECK(error==0);
    central.set_CurrentPeripheral(unnamed.value);{std::lock_guard<std::mutex> lock(control->mutex);CHECK(control->selectedId==unnamed.id);}central.peripheral_Connect();CHECK(central.peripheral_Is_Connected());CHECK(error==0);
}
void destruction(){
    auto central=std::make_unique<SimpleCommKitBleCentral>();auto control=fake;
    central->set_CurrentAdapter(central->get_Adapters().front());
    std::promise<void> gone;std::atomic<int> callbacks{0};
    central->adapter_Set_Callback_On_Scan_Start([&]{++callbacks;central.reset();gone.set_value();});
    // Invoke on another thread: destruction may race the return from a call.
    auto raw=central.get();raw->adapter_Scan_Start();
    CHECK(gone.get_future().wait_for(std::chrono::seconds(2))==std::future_status::ready);
    until([&]{return control->stopped.load();});control->event(EventType::ScanStarted);CHECK(callbacks==1);
}
void reentryAndCancellation(){
    SimpleCommKitBleCentral central;auto control=fake;std::atomic<ErrorCode> error{0};
    central.set_Callback_Error([&](ErrorCode e){error=e;});central.set_CurrentAdapter(central.get_Adapters().front());
    central.adapter_Set_Callback_On_Scan_Start([&]{CHECK(central.adapter_Is_Powered());central.peripheral_Read("180d","2a37");central.adapter_Scan_Stop();});
    central.adapter_Scan_Start();until([&]{return error==ErrorCodes::SimpleCommKitBleReentrantOperation;});until([&]{return !control->scanning;});
    control->blockRead=true;std::thread caller([&]{central.peripheral_Read("180d","2a37");});until([&]{return control->reading.load();});
    central.peripheral_Disconnect();caller.join();until([&]{return error==ErrorCodes::SimpleCommKitBleCancelled;});
}
void destroyWhileReading(){
    auto central=std::make_unique<SimpleCommKitBleCentral>();auto control=fake;
    control->blockRead=true;auto raw=central.get();
    std::thread caller([raw]{raw->peripheral_Read("180d","2a37");});
    until([&]{return control->reading.load();});
    central.reset();caller.join();CHECK(control->stopped);
}
int main(){try{runtime();facade();platformAddressSelection();destruction();reentryAndCancellation();destroyWhileReading();std::cout<<"native BLE contract tests passed\n";}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
