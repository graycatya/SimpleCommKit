#include "../BleBackend.h"
#include "AndroidScanState.h"
#include <jni.h>
#include <map>
#include <mutex>
#include <sstream>

namespace SimpleCommKit::Ble::AndroidDetail {
std::mutex runtimeMutex;
JavaVM* vm = nullptr;
jclass bridgeClass = nullptr; // Process lifetime; Java retains only Application Context.
struct Env {
    JavaVM* machine;
    JNIEnv* value = nullptr;
    bool attached = false;
    Env() {
        { std::lock_guard<std::mutex> lock(runtimeMutex); machine = vm; }
        if (!machine) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable,
            "Load native library and call NativeBle.initialize(Application Context) first");
        const auto status = machine->GetEnv(reinterpret_cast<void**>(&value), JNI_VERSION_1_6);
        if (status == JNI_EDETACHED) {
            if (machine->AttachCurrentThread(&value, nullptr) != JNI_OK)
                throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
            attached = true;
        } else if (status != JNI_OK) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
    }
    ~Env() { if (attached) machine->DetachCurrentThread(); }
    void check() {
        if (!value->ExceptionCheck()) return;
        auto exception = value->ExceptionOccurred(); value->ExceptionClear();
        auto security = value->FindClass("java/lang/SecurityException");
        const bool denied = security && value->IsInstanceOf(exception, security);
        ErrorCode code = denied ? ErrorCodes::SimpleCommKitBlePermissionDenied : ErrorCodes::SimpleCommKitBleBackendUnavailable;
        auto type = value->GetObjectClass(exception);
        auto getter = value->GetMethodID(type, "getErrorCode", "()I");
        if (value->ExceptionCheck()) value->ExceptionClear();
        if (getter) code = static_cast<ErrorCode>(value->CallIntMethod(exception, getter));
        value->DeleteLocalRef(type);
        if (security) value->DeleteLocalRef(security);
        value->DeleteLocalRef(exception);
        if (value->ExceptionCheck()) value->ExceptionClear();
        throw Failure(code);
    }
};
std::mutex registryMutex;
std::map<jlong, std::weak_ptr<State>> registry;
jlong nextHandle = 0;
std::shared_ptr<State> find(jlong handle) {
    std::lock_guard<std::mutex> lock(registryMutex);
    auto it = registry.find(handle);
    return it == registry.end() ? nullptr : it->second.lock();
}
std::string string(JNIEnv* env, jstring value) {
    if (!value) return {};
    auto chars = env->GetStringUTFChars(value, nullptr);
    if (!chars) return {};
    std::string result(chars); env->ReleaseStringUTFChars(value, chars); return result;
}
[[noreturn]] void unsupported(const Operation& op) {
    op.check(); throw Failure(ErrorCodes::SimpleCommKitBleNotSupported);
}
class AndroidBackend final : public IBleCentralBackend {
    std::shared_ptr<State> state = std::make_shared<State>();
    // Serializes Java reference access against concurrent shutdown.
    std::mutex bridgeMutex;
    jobject bridge = nullptr;
    jlong handle;
    bool selected = false;
    std::string peripheralAddress;
    bool stopped = false;
    void ensure(Env& env, const Operation& op) {
        op.check();
        if (stopped) throw Failure(ErrorCodes::SimpleCommKitBleCancelled);
        if (bridge) return;
        auto constructor = env.value->GetMethodID(bridgeClass, "<init>", "(J)V"); env.check();
        auto local = env.value->NewObject(bridgeClass, constructor, handle); env.check();
        bridge = env.value->NewGlobalRef(local); env.value->DeleteLocalRef(local); env.check();
        if (!bridge) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
    }
    bool boolean(const char* method, const Operation& op) {
        std::lock_guard<std::mutex> lock(bridgeMutex); Env env; ensure(env, op);
        auto id = env.value->GetMethodID(bridgeClass, method, "()Z"); env.check();
        auto value = env.value->CallBooleanMethod(bridge, id); env.check(); op.check(); return value;
    }
    void requireAdapter(const Operation& op) {
        op.check();
        if (!selected) throw Failure(ErrorCodes::SimpleCommKitBleCurrentAdapterNotSet);
    }
    std::vector<uint8_t> transact(int action, const std::string& service, const std::string& characteristic,
        const std::string& descriptor, const std::vector<uint8_t>& data, int mode, const Operation& op) {
        Env env;
        jobject local;
        { std::lock_guard<std::mutex> lock(bridgeMutex); ensure(env, op); local = env.value->NewLocalRef(bridge); }
        struct LocalFrame {
            JNIEnv* env; jobject local;
            ~LocalFrame() { env->DeleteLocalRef(local); env->PopLocalFrame(nullptr); }
        };
        if (env.value->PushLocalFrame(16) != JNI_OK) { env.value->DeleteLocalRef(local); env.check(); }
        LocalFrame cleanup{env.value, local};
        uint64_t token;
        { std::lock_guard<std::mutex> lock(state->mutex); state->pendingCancelled = op.cancelled; token = state->linkEpoch; }
        auto method = env.value->GetMethodID(bridgeClass, "transact",
            "(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;[BIJJ)[B"); env.check();
        auto s = env.value->NewStringUTF(service.c_str());
        auto c = env.value->NewStringUTF(characteristic.c_str());
        auto d = env.value->NewStringUTF(descriptor.c_str());
        auto bytes = env.value->NewByteArray(static_cast<jsize>(data.size())); env.check();
        if (!data.empty()) env.value->SetByteArrayRegion(bytes, 0, static_cast<jsize>(data.size()), reinterpret_cast<const jbyte*>(data.data()));
        const auto remaining = std::chrono::duration_cast<std::chrono::milliseconds>(op.deadline - Clock::now()).count();
        auto result = static_cast<jbyteArray>(env.value->CallObjectMethod(local, method, action, s, c, d, bytes,
            mode, static_cast<jlong>(remaining), static_cast<jlong>(token)));
        try { env.check(); op.check(); }
        catch (...) {
            // An outstanding Android callback has no request ID. Close its GATT
            // domain on any failed transaction before permitting another request.
            auto discarded = env.value->CallObjectMethod(local, method, 2, s, c, d, bytes,
                0, static_cast<jlong>(1), static_cast<jlong>(token));
            if (discarded) env.value->DeleteLocalRef(discarded);
            if (env.value->ExceptionCheck()) env.value->ExceptionClear();
            std::lock_guard<std::mutex> lock(state->mutex);
            const bool wasLinked = state->linked;
            state->linked = false; state->subscriptions.clear(); ++state->linkEpoch;
            if (action != 2 && wasLinked) {
                state->emit(Event(EventType::Disconnected, state->linkGeneration));
            }
            throw;
        }
        std::vector<uint8_t> output;
        if (result) {
            output.resize(env.value->GetArrayLength(result));
            if (!output.empty()) env.value->GetByteArrayRegion(result, 0, static_cast<jsize>(output.size()), reinterpret_cast<jbyte*>(output.data()));
        }
        env.check(); return output;
    }
    void requirePeripheral(const Operation& op) {
        requireAdapter(op);
        if (peripheralAddress.empty()) throw Failure(ErrorCodes::SimpleCommKitBleCurrentPeripheralNotSet);
    }
public:
    AndroidBackend() {
        std::lock_guard<std::mutex> lock(registryMutex);
        handle = ++nextHandle; registry.emplace(handle, state);
    }
    ~AndroidBackend() override { shutdown(); }
    Capabilities capabilities() const override { Capabilities c; c.mtu = true; return c; }
    void setEventSink(EventSink sink) override {
        std::lock_guard<std::mutex> lock(state->mutex); state->sink = std::move(sink);
    }
    void shutdown() noexcept override {
        { std::lock_guard<std::mutex> lock(state->mutex);
          state->closing = true; state->scanning = false; state->sink = {}; ++state->epoch;
          state->linked = false; ++state->linkEpoch; state->subscriptions.clear();
          if (state->pendingCancelled) state->pendingCancelled->store(true); }
        { std::lock_guard<std::mutex> lock(registryMutex); registry.erase(handle); }
        std::lock_guard<std::mutex> lock(bridgeMutex);
        stopped = true;
        if (!bridge) return;
        try {
            Env env;
            auto close = env.value->GetMethodID(bridgeClass, "close", "()V"); env.check();
            env.value->CallVoidMethod(bridge, close);
            if (env.value->ExceptionCheck()) env.value->ExceptionClear();
            env.value->DeleteGlobalRef(bridge); bridge = nullptr;
        } catch (...) { /* JVM teardown: no exceptions may escape destruction. */ }
    }
    bool enabled(const Operation& op) override { return boolean("enabled", op); }
    std::vector<Adapter> adapters(const Operation& op) override {
        if (!boolean("available", op)) return {};
        return {{"android:default", {"Android Bluetooth", ""}}};
    }
    void selectAdapter(const std::string& id, const Operation& op) override {
        if (id != "android:default" || adapters(op).empty())
            throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument);
        selected = true;
    }
    bool powered(const Operation& op) override { requireAdapter(op); return enabled(op); }
    void setPowered(bool, const Operation& op) override { unsupported(op); }
    void startScan(const Operation& op) override {
        requireAdapter(op);
        std::lock_guard<std::mutex> bridgeLock(bridgeMutex); Env env; ensure(env, op);
        uint64_t epoch;
        { std::lock_guard<std::mutex> lock(state->mutex);
          if (state->scanning) return;
          if (state->closing) throw Failure(ErrorCodes::SimpleCommKitBleCancelled);
          state->devices.clear(); state->generation = op.generation;
          state->scanning = true; epoch = ++state->epoch;
          state->emit(Event(EventType::ScanStarted, op.generation)); }
        try {
            auto id = env.value->GetMethodID(bridgeClass, "startScan", "(J)V"); env.check();
            env.value->CallVoidMethod(bridge, id, static_cast<jlong>(epoch)); env.check(); op.check();
        } catch (...) {
            // Invalidate before stopping Java to reject in-flight callbacks.
            { std::lock_guard<std::mutex> lock(state->mutex);
              state->scanning = false; ++state->epoch;
              state->emit(Event(EventType::ScanStopped, op.generation)); }
            auto stop = env.value->GetMethodID(bridgeClass, "stopScan", "()V");
            if (stop) env.value->CallVoidMethod(bridge, stop);
            if (env.value->ExceptionCheck()) env.value->ExceptionClear();
            throw;
        }
    }
    void stopScan(const Operation& op) override {
        requireAdapter(op);
        { std::lock_guard<std::mutex> lock(state->mutex);
          if (!state->scanning) return;
          state->scanning = false; ++state->epoch;
          state->emit(Event(EventType::ScanStopped, state->generation)); }
        std::lock_guard<std::mutex> bridgeLock(bridgeMutex); Env env; ensure(env, op);
        auto id = env.value->GetMethodID(bridgeClass, "stopScan", "()V"); env.check();
        env.value->CallVoidMethod(bridge, id); env.check(); op.check();
    }
    bool scanning(const Operation& op) override {
        // The facade queries scanning before selecting its first adapter.
        op.check(); std::lock_guard<std::mutex> lock(state->mutex); return state->scanning;
    }
    std::vector<Peripheral> peripherals(DeviceFilter filter, const Operation& op) override {
        requireAdapter(op); if (filter != DeviceFilter::All) unsupported(op);
        std::lock_guard<std::mutex> lock(state->mutex);
        std::vector<Peripheral> result; for (const auto& entry : state->devices) result.push_back(entry.second);
        return result;
    }
    void selectPeripheral(const std::string& id, const Operation& op) override {
        requireAdapter(op); std::lock_guard<std::mutex> lock(state->mutex);
        if (!state->devices.count(id)) throw Failure(ErrorCodes::SimpleCommKitBleInvalidArgument);
        peripheralAddress = state->devices.at(id).value.address;
    }
    void connect(const Operation& op) override {
        requirePeripheral(op);
        uint64_t token;
        { std::lock_guard<std::mutex> lock(state->mutex);
          if (state->linked) return;
          token = ++state->linkEpoch; state->linkGeneration = op.generation; }
        transact(1, peripheralAddress, "", "", {}, 0, op);
        std::lock_guard<std::mutex> lock(state->mutex);
        if (state->closing) throw Failure(ErrorCodes::SimpleCommKitBleCancelled);
        if (state->linkEpoch != token) throw Failure(ErrorCodes::SimpleCommKitBleBackendUnavailable);
        state->linked = true; state->emit(Event(EventType::Connected, op.generation));
    }
    void disconnect(const Operation& op) override {
        requirePeripheral(op);
        bool wasLinked;
        { std::lock_guard<std::mutex> lock(state->mutex); wasLinked = state->linked;
          state->linked = false; ++state->linkEpoch; state->subscriptions.clear(); }
        transact(2, "", "", "", {}, 0, op);
        if (wasLinked) { std::lock_guard<std::mutex> lock(state->mutex); state->emit(Event(EventType::Disconnected, op.generation)); }
    }
    bool connected(const Operation& op) override {
        requirePeripheral(op); std::lock_guard<std::mutex> lock(state->mutex); return state->linked;
    }
    bool connectable(const Operation& op) override { unsupported(op); }
    bool paired(const Operation& op) override { unsupported(op); }
    void unpair(const Operation& op) override { unsupported(op); }
    int16_t txPower(const Operation& op) override { unsupported(op); }
    uint16_t mtu(const Operation& op) override {
        requirePeripheral(op); auto bytes = transact(8, "", "", "", {}, 0, op);
        if (bytes.size() != 2) throw Failure(ErrorCodes::SimpleCommKitBleGattError);
        return uint16_t(bytes[0]) | (uint16_t(bytes[1]) << 8);
    }
    std::vector<SimpleCommKitBleService> services(const Operation& op) override {
        requirePeripheral(op); auto bytes = transact(3, "", "", "", {}, 0, op);
        std::istringstream input(std::string(bytes.begin(), bytes.end()));
        std::vector<SimpleCommKitBleService> output; std::string kind, uuid; int props;
        while (input >> kind >> uuid) {
            if (kind == "S") { SimpleCommKitBleService s{}; s.uuid = canonicalUuid(uuid); output.push_back(s); }
            else if (kind == "C" && !output.empty() && (input >> props)) {
                SimpleCommKitBleCharacteristic c{}; c.uuid = canonicalUuid(uuid);
                c.can_read = props & 2; c.can_write_request = props & 8; c.can_write_command = props & 4;
                c.can_notify = props & 16; c.can_indicate = props & 32;
                if (c.can_read) c.capabilities.push_back("read");
                if (c.can_write_request) c.capabilities.push_back("write_request");
                if (c.can_write_command) c.capabilities.push_back("write_command");
                if (c.can_notify) c.capabilities.push_back("notify");
                if (c.can_indicate) c.capabilities.push_back("indicate");
                output.back().characteristics.push_back(c);
            } else if (kind == "D" && !output.empty() && !output.back().characteristics.empty())
                output.back().characteristics.back().descriptors_uuid.push_back(canonicalUuid(uuid));
            else throw Failure(ErrorCodes::SimpleCommKitBleGattError);
        }
        return output;
    }
    std::vector<uint8_t> read(const std::string& s, const std::string& c, const std::string& d, const Operation& op) override {
        requirePeripheral(op); return transact(4, canonicalUuid(s), canonicalUuid(c), d.empty() ? "" : canonicalUuid(d), {}, 0, op);
    }
    void write(const std::string& s, const std::string& c, const std::string& d, const std::vector<uint8_t>& data, WriteType type, const Operation& op) override {
        requirePeripheral(op); transact(5, canonicalUuid(s), canonicalUuid(c), d.empty() ? "" : canonicalUuid(d), data, type == WriteType::Command ? 1 : 0, op);
    }
    void subscribe(const std::string& s, const std::string& c, SubscriptionType type, const Operation& op) override {
        requirePeripheral(op); auto service = canonicalUuid(s), characteristic = canonicalUuid(c);
        { std::lock_guard<std::mutex> lock(state->mutex); state->subscriptions[{service, characteristic}] = true; }
        try { transact(6, service, characteristic, "", {}, type == SubscriptionType::Indicate ? 1 : 0, op); }
        catch (...) { std::lock_guard<std::mutex> lock(state->mutex); state->subscriptions.erase({service, characteristic}); throw; }
    }
    void unsubscribe(const std::string& s, const std::string& c, const Operation& op) override {
        requirePeripheral(op); auto service = canonicalUuid(s), characteristic = canonicalUuid(c);
        { std::lock_guard<std::mutex> lock(state->mutex); state->subscriptions.erase({service, characteristic}); }
        transact(7, service, characteristic, "", {}, 0, op);
    }
};
} // namespace SimpleCommKit::Ble::AndroidDetail

extern "C" JNIEXPORT void JNICALL
Java_org_simplecommkit_ble_NativeBle_nativeInitialize(JNIEnv* env, jclass type) {
    using namespace SimpleCommKit::Ble::AndroidDetail;
    std::lock_guard<std::mutex> lock(runtimeMutex);
    if (bridgeClass) return;
    if (env->GetJavaVM(&vm) != JNI_OK) { vm = nullptr; return; }
    bridgeClass = static_cast<jclass>(env->NewGlobalRef(type));
}
extern "C" JNIEXPORT void JNICALL
Java_org_simplecommkit_ble_NativeBle_nativeAdvertisement(JNIEnv* env, jclass,
    jlong handle, jlong epoch, jstring address, jstring name, jint rssi,
    jintArray companies, jobjectArray manufacturer) {
    using namespace SimpleCommKit;
    using namespace SimpleCommKit::Ble;
    using namespace SimpleCommKit::Ble::AndroidDetail;
    try {
        auto state = find(handle); if (!state) return;
        Peripheral p; p.value.address = string(env, address);
        if (p.value.address.empty()) return;
        p.id = "android:" + p.value.address; p.name = string(env, name);
        p.value.address_type = UNSPECIFIED; p.value.rssi = static_cast<int16_t>(rssi);
        const jsize n = env->GetArrayLength(companies);
        if (n != env->GetArrayLength(manufacturer)) return;
        for (jsize i = 0; i < n; ++i) {
            jint company; env->GetIntArrayRegion(companies, i, 1, &company);
            auto bytes = static_cast<jbyteArray>(env->GetObjectArrayElement(manufacturer, i));
            if (!bytes) continue;
            auto& data = p.value.manufacturer[static_cast<uint16_t>(company)];
            data.resize(env->GetArrayLength(bytes));
            env->GetByteArrayRegion(bytes, 0, static_cast<jsize>(data.size()), reinterpret_cast<jbyte*>(data.data()));
            env->DeleteLocalRef(bytes);
        }
        if (env->ExceptionCheck()) return;
        state->advertisement(static_cast<uint64_t>(epoch), std::move(p));
    } catch (...) { /* Never unwind C++ exceptions through ART. */ }
}
extern "C" JNIEXPORT void JNICALL
Java_org_simplecommkit_ble_NativeBle_nativeScanFailed(JNIEnv*, jclass, jlong handle, jlong epoch, jint reason) {
    using namespace SimpleCommKit;
    using namespace SimpleCommKit::Ble;
    using namespace SimpleCommKit::Ble::AndroidDetail;
    try {
        auto state = find(handle); if (!state) return;
        std::lock_guard<std::mutex> lock(state->mutex);
        if (state->closing || !state->scanning || state->epoch != static_cast<uint64_t>(epoch)) return;
        state->scanning = false; ++state->epoch;
        Event error(EventType::Error, state->generation);
        error.error = reason == -1 ? ErrorCodes::SimpleCommKitBlePermissionDenied :
            reason == 4 ? ErrorCodes::SimpleCommKitBleNotSupported : ErrorCodes::SimpleCommKitBleBackendUnavailable;
        state->emit(std::move(error)); state->emit(Event(EventType::ScanStopped, state->generation));
    } catch (...) {}
}
namespace SimpleCommKit::Ble {
std::unique_ptr<IBleCentralBackend> createBackend() { return std::make_unique<AndroidDetail::AndroidBackend>(); }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_org_simplecommkit_ble_NativeBle_nativeCancelled(JNIEnv*, jclass, jlong handle) {
    using namespace SimpleCommKit::Ble::AndroidDetail;
    auto state = find(handle); if (!state) return JNI_TRUE;
    std::lock_guard<std::mutex> lock(state->mutex);
    return state->closing || (state->pendingCancelled && state->pendingCancelled->load());
}
extern "C" JNIEXPORT void JNICALL
Java_org_simplecommkit_ble_NativeBle_nativeGattEvent(JNIEnv* env, jclass, jlong handle,
    jlong token, jint type, jstring service, jstring characteristic, jbyteArray data) {
    using namespace SimpleCommKit;
    using namespace SimpleCommKit::Ble;
    using namespace SimpleCommKit::Ble::AndroidDetail;
    try {
        auto state = find(handle); if (!state) return;
        const auto s = string(env, service), c = string(env, characteristic);
        std::vector<uint8_t> bytes(env->GetArrayLength(data));
        if (!bytes.empty()) env->GetByteArrayRegion(data, 0, static_cast<jsize>(bytes.size()), reinterpret_cast<jbyte*>(bytes.data()));
        if (env->ExceptionCheck()) return;
        state->gattEvent(static_cast<uint64_t>(token), type, s, c, std::move(bytes));
    } catch (...) {}
}
