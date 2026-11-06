#include <jni.h>
#include "SimpleCommKitBleCentral.h"
#include <atomic>
#include <chrono>
#include <sstream>
#include <thread>
#include <iomanip>
#include <mutex>
#include <algorithm>
using namespace SimpleCommKit;
namespace {
std::mutex outputMutex;
std::string output;
class ProbeMessage {
    std::ostringstream text;
public:
    ~ProbeMessage() { std::lock_guard<std::mutex> lock(outputMutex); output += text.str(); }
    template<class T> ProbeMessage& operator<<(const T& value) { text << value; return *this; }
    ProbeMessage& operator<<(std::ostream& (*fn)(std::ostream&)) { fn(text); return *this; }
};
ProbeMessage consoleOut() { return {}; }
ProbeMessage consoleError() { return {}; }
}
#include "Im948ReadProbe.hpp"
extern "C" JNIEXPORT jstring JNICALL
Java_org_simplecommkit_ble_probe_MainActivity_nativeUninitialized(JNIEnv* env, jclass) {
    std::string result = "FAIL uninitialized backend";
    try {
        auto error = std::make_shared<std::atomic<ErrorCode>>(0);
        SimpleCommKitBleCentral central;
        central.set_Callback_Error([error](auto code) { *error = code; });
        const auto adapters = central.get_Adapters();
        for (int i = 0; i < 100 && !error->load(); ++i)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        if (adapters.empty() && *error == ErrorCodes::SimpleCommKitBleBackendUnavailable)
            result = "PASS uninitialized backend reports BackendUnavailable";
        else result += " error=" + std::to_string(error->load());
    } catch (const std::exception& e) { result += std::string(": ") + e.what(); }
      catch (...) { result += ": unknown C++ exception"; }
    return env->NewStringUTF(result.c_str());
}
extern "C" JNIEXPORT jstring JNICALL
Java_org_simplecommkit_ble_probe_MainActivity_nativeScan(JNIEnv* env, jclass) {
    std::ostringstream out;
    try {
        struct Capture { std::atomic<unsigned> error{0}, starts{0}, stops{0}, advertisements{0}; };
        auto capture = std::make_shared<Capture>();
        SimpleCommKitBleCentral central;
        central.set_Callback_Error([capture](auto e) { capture->error = e; });
        central.adapter_Set_Callback_On_Scan_Start([capture] { ++capture->starts; });
        central.adapter_Set_Callback_On_Scan_Stop([capture] { ++capture->stops; });
        central.adapter_Set_Callback_On_Scan_Found([capture](auto) { ++capture->advertisements; });
        central.adapter_Set_Callback_On_Scan_Updated([capture](auto) { ++capture->advertisements; });
        auto adapters = central.get_Adapters();
        out << "Adapters=" << adapters.size() << '\n';
        if (adapters.empty() || capture->error) throw std::runtime_error("adapter unavailable");
        central.set_CurrentAdapter(adapters.front());
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        out << "Adapter selected error=" << capture->error.load() << '\n';
        if (!central.adapter_Is_Powered() || capture->error) throw std::runtime_error("Bluetooth disabled or permission denied");
        bool foundAny = false;
        for (unsigned cycle = 1; cycle <= 2; ++cycle) {
            central.adapter_Scan_For(4000);
            central.adapter_Scan_Stop(); // Idempotent stop.
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            const auto count = capture->advertisements.load();
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            if (capture->error || central.adapter_Scan_Is_Active() || capture->starts != cycle ||
                capture->stops != cycle || count != capture->advertisements)
                throw std::runtime_error("scan lifecycle/error/late callback check failed");
            auto devices = central.adapter_Get_Scan_Results();
            foundAny |= !devices.empty();
            out << "PASS scan " << cycle << ": devices=" << devices.size() << " callbacks=" << count << '\n';
            for (const auto& device : devices)
                out << "  " << device.address << " " << device.identifier << " RSSI=" << device.rssi
                    << " manufacturers=" << device.manufacturer.size() << '\n';
        }
        for (int i = 0; i < 20; ++i) {
            SimpleCommKitBleCentral temporary;
            if (temporary.get_Adapters().empty()) throw std::runtime_error("repeated construction failed");
        }
        out << "PASS 20 create/enumerate/destroy cycles\n";
        if (!foundAny) throw std::runtime_error("no real advertisements received");
        out << "PASS Kotlin/JNI/public C++ scan integration\n";
    } catch (const std::exception& e) { out << "FAIL " << e.what() << '\n'; }
      catch (...) { out << "FAIL unknown native exception\n"; }
    return env->NewStringUTF(out.str().c_str());
}

extern "C" JNIEXPORT jstring JNICALL
Java_org_simplecommkit_ble_probe_MainActivity_nativeIm948(JNIEnv* env, jclass, jstring target) {
    { std::lock_guard<std::mutex> lock(outputMutex); output.clear(); }
    const char* chars = env->GetStringUTFChars(target, nullptr);
    if (!chars) return nullptr;
    const std::string address(chars); env->ReleaseStringUTFChars(target, chars);
    try {
        SimpleCommKitBleCentral central;
        central.set_Callback_Error([](auto code) { consoleError() << "[BLE ERROR] " << code << std::endl; });
        auto adapters = central.get_Adapters();
        if (adapters.empty()) throw std::runtime_error("No Bluetooth adapter");
        central.set_CurrentAdapter(adapters.front());
        central.adapter_Scan_For(15000);
        auto devices = central.adapter_Get_Scan_Results();
        const auto found = std::find_if(devices.begin(), devices.end(), [&](const auto& p) { return p.address == address; });
        if (found == devices.end()) throw std::runtime_error("IM948 not advertising; wake device and retry");
        consoleOut() << "[ANDROID IM948] " << found->identifier << " " << found->address << std::endl;
        central.set_CurrentPeripheral(*found);
        central.peripheral_Connect();
        if (!central.peripheral_Is_Connected()) throw std::runtime_error("IM948 connection failed");
        consoleOut() << "[ANDROID MTU] " << central.peripheral_Get_Mtu() << std::endl;
        auto services = central.peripheral_Services();
        consoleOut() << "[ANDROID SERVICES] " << services.size() << std::endl;
        const bool passed = testIm948(central, services);
        consoleOut() << (passed ? "[ANDROID IM948 PASS]" : "[ANDROID IM948 FAIL]") << std::endl;
    } catch (const std::exception& e) { consoleError() << "[ANDROID IM948 FAIL] " << e.what() << std::endl; }
      catch (...) { consoleError() << "[ANDROID IM948 FAIL] Unknown exception" << std::endl; }
    std::lock_guard<std::mutex> lock(outputMutex);
    return env->NewStringUTF(output.c_str());
}
