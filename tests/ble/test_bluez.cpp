#include "SimpleCommKitBleCentral.h"
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
using namespace SimpleCommKit;
#define CHECK(test) do{if(!(test))throw std::runtime_error("Check failed: " #test);}while(false)
template<class P> void untilImpl(P predicate,int line){auto end=std::chrono::steady_clock::now()+std::chrono::seconds(3);while(!predicate()){if(std::chrono::steady_clock::now()>=end)throw std::runtime_error("Timed out at line "+std::to_string(line));std::this_thread::sleep_for(std::chrono::milliseconds(1));}}
#define until(...) untilImpl(__VA_ARGS__,__LINE__)
int main(int argc,char**){try{
    if(argc>1) {
        SimpleCommKitBleCentral unavailable;std::atomic<ErrorCode> error{0};
        unavailable.set_Callback_Error([&](ErrorCode value){error=value;});
        CHECK(unavailable.get_Adapters().empty());
        until([&]{return error==ErrorCodes::SimpleCommKitBleBackendUnavailable;});
        CHECK(!SimpleCommKitBleCentral::Bluetooth_Enabled());
        std::cout<<"BlueZ unavailable error handling passed\n";return 0;
    }
    SimpleCommKitBleCentral central;std::atomic<ErrorCode> error{0};std::atomic<int> found{0},updated{0},notifications{0},connected{0},disconnected{0};
    central.set_Callback_Error([&](ErrorCode e){error=e;});
    auto adapters=central.get_Adapters();CHECK(adapters.size()==1);CHECK(SimpleCommKitBleCentral::Bluetooth_Enabled());
    central.set_CurrentAdapter(adapters.front());CHECK(central.adapter_Is_Powered());CHECK(central.adapter_Get_Paired_Peripherals().size()==1);
    central.adapter_Set_Callback_On_Scan_Found([&](auto){++found;});central.adapter_Set_Callback_On_Scan_Updated([&](auto){++updated;});
    central.adapter_Scan_Start();until([&]{return found==1;});until([&]{return updated>0;});
    central.adapter_Scan_Stop();auto devices=central.adapter_Get_Scan_Results();CHECK(devices.size()==1);CHECK(devices.front().manufacturer.at(1)==std::vector<uint8_t>({0,255}));
    central.set_CurrentPeripheral(devices.front());central.peripheral_Set_Callback_On_Connected([&]{++connected;});central.peripheral_Set_Callback_On_Disconnected([&]{++disconnected;});
    central.peripheral_Connect();CHECK(central.peripheral_Is_Connected());until([&]{return connected==1;});
    auto services=central.peripheral_Services();CHECK(services.size()==1);CHECK(services.front().data==std::vector<uint8_t>({9,0}));CHECK(central.peripheral_Get_Tx_Power()==-9);CHECK(services.front().characteristics.front().can_notify);
    CHECK(services.front().characteristics.front().descriptors_uuid.size()==1);
    CHECK(central.peripheral_Read("180d","2a37")==std::vector<uint8_t>({0,1,255}));
    central.peripheral_Write_Request("180d","2a37",{255,0,2});CHECK(central.peripheral_Read("180d","2a37")==std::vector<uint8_t>({255,0,2}));
    central.peripheral_Write_Command("180d","2a37",{2,0,255});CHECK(central.peripheral_Read("180d","2a37")==std::vector<uint8_t>({2,0,255}));
    CHECK(central.peripheral_Read("180d","2a37","2901")==std::vector<uint8_t>({0,1,255}));
    central.peripheral_Write("180d","2a37","2901",{5,0});CHECK(central.peripheral_Read("180d","2a37","2901")==std::vector<uint8_t>({5,0}));
    central.peripheral_Notify("180d","2a37",[&](auto bytes){if(bytes==std::vector<uint8_t>({2,0,255}))++notifications;});
    until([&]{return notifications>=2;});central.peripheral_Unsubscribe("180d","2a37");int count=notifications;
    std::this_thread::sleep_for(std::chrono::milliseconds(100));CHECK(notifications==count);CHECK(error==0);
    central.peripheral_Disconnect();until([&]{return disconnected==1;});CHECK(error==0);
    central.peripheral_Connect();until([&]{return connected==2;});
    central.peripheral_Write_Request("180d","2a37",{238});until([&]{return error==ErrorCodes::SimpleCommKitBlePermissionDenied;});
    central.peripheral_Write_Request("180d","2a37",{221});CHECK(central.peripheral_Read("180d","2a37").empty());
    until([&]{return error==ErrorCodes::SimpleCommKitBleAmbiguousIdentity;});
    central.peripheral_Disconnect();until([&]{return disconnected==2;});CHECK(!central.peripheral_Is_Connected());
    CHECK(central.peripheral_Get_Mtu()==0);until([&]{return error==ErrorCodes::SimpleCommKitBleNotSupported;});
    central.peripheral_Unpair();CHECK(central.adapter_Get_Paired_Peripherals().empty());
    CHECK(!central.peripheral_Is_Connected());until([&]{return error==ErrorCodes::SimpleCommKitBleBackendUnavailable;});
    std::cout<<"BlueZ D-Bus integration passed\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
