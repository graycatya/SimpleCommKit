#pragma once
// Explicit opt-in probe for the public IM948 protocol. Queries only: no sensor
// calibration, persistent settings, or active-reporting changes. Optional sensor
// subscription changes are restored and verified before disconnect.
// https://www.yuque.com/cxqwork/lkw3sg/yqa3e0
inline bool readIm948(SimpleCommKit::SimpleCommKitBleCentral& central,
                      const std::vector<SimpleCommKit::SimpleCommKitBleService>& services, bool sensorFields = false) {
    const std::string service = "0000ae30-0000-1000-8000-00805f9b34fb";
    const std::string tx = "0000ae01-0000-1000-8000-00805f9b34fb";
    const std::string rx = "0000ae02-0000-1000-8000-00805f9b34fb";
    bool canSend = false, canReceive = false;
    for (const auto& s : services) if (s.uuid == service) {
        for (const auto& c : s.characteristics) {
            if (c.uuid == tx) canSend = c.can_write_command;
            if (c.uuid == rx) canReceive = c.can_notify;
        }
    }
    if (!canSend || !canReceive) {
        consoleError() << "[IM948 FAIL] Expected AE30/AE01 command and AE30/AE02 notify capabilities" << std::endl;
        return false;
    }
    struct Capture {
        std::mutex mutex;
        std::vector<std::vector<uint8_t>> frames;
        std::atomic<SimpleCommKit::ErrorCode> error{0};
    };
    auto capture = std::make_shared<Capture>();
    central.set_Callback_Error([capture](auto error) {
        capture->error = error;
        consoleError() << "[BLE ERROR] " << error << " : "
            << SimpleCommKit::SimpleCommKitErrorMap::GetErrorDescription(error) << std::endl;
    });
    central.peripheral_Notify(service, rx, [capture](auto data) {
        std::ostringstream out; out << "[IM948 RX] " << data.size() << " bytes: ";
        for (auto byte : data) out << std::hex << std::setfill('0') << std::setw(2) << unsigned(byte) << ' ';
        consoleOut() << out.str() << std::endl;
        std::lock_guard<std::mutex> lock(capture->mutex);
        capture->frames.push_back(std::move(data));
    });
    auto query = [&](uint8_t tag, size_t minLength,
                     const std::vector<uint8_t>& command = {}) -> std::vector<uint8_t> {
        size_t start;
        { std::lock_guard<std::mutex> lock(capture->mutex); start = capture->frames.size(); }
        consoleOut() << "[IM948 TX COMMAND] " << unsigned(tag) << std::endl;
        central.peripheral_Write_Command(service, tx, command.empty() ? std::vector<uint8_t>{tag} : command);
        const auto end = std::chrono::steady_clock::now() + std::chrono::seconds(4);
        while (std::chrono::steady_clock::now() < end && !capture->error) {
            { std::lock_guard<std::mutex> lock(capture->mutex);
              for (size_t i = start; i < capture->frames.size(); ++i)
                  if (capture->frames[i].size() >= minLength && capture->frames[i][0] == tag) return capture->frames[i]; }
            if (!central.peripheral_Is_Connected()) break;
            std::this_thread::sleep_for(std::chrono::milliseconds(10));
        }
        consoleError() << "[IM948 FAIL] No complete response for tag " << unsigned(tag) << std::endl;
        return {};
    };
    bool passed = !capture->error && !query(0x29, 1).empty(); // Keep this BLE session alive.
    auto status = passed ? query(0x10, 33) : std::vector<uint8_t>{};
    passed = passed && !status.empty();
    if (!status.empty()) {
        const unsigned mask = unsigned(status[9]) | (unsigned(status[10]) << 8);
        consoleOut() << "[IM948 STATUS] battery=" << unsigned(status[12]) << "% voltage="
            << (unsigned(status[13]) | (unsigned(status[14]) << 8)) << "mV sensor_on="
            << bool(status[4] & 8) << " subscription_mask=" << mask << std::endl;
    }
    struct Restore {
        std::function<void()> action;
        ~Restore() { if (action) { try { action(); } catch (...) {} } }
    } restore;
    if (passed && sensorFields) {
        // 0x12 copies existing parameters byte-for-byte, changing only the
        // volatile subscription bitmap (gravity acceleration, gyro, Euler).
        std::vector<uint8_t> original(status.begin(), status.begin() + 11);
        original[0] = 0x12;
        auto temporary = original; temporary[9] = 0x46; temporary[10] = 0;
        restore.action = [&, original] {
            consoleOut() << "[IM948 RESTORE] Restoring original subscription and parameters" << std::endl;
            if (!central.peripheral_Is_Connected()) { passed = false; return; }
            // Wait for the parameter command acknowledgement before querying status.
            // Write Command completion alone does not mean the device has applied it.
            const bool acknowledged = !query(0x12, 1, original).empty();
            auto restored = query(0x10, 33);
            // Status byte 4 contains live flags (sensor/report state). Compare
            // the three configuration bits and every other parameter byte.
            bool same = acknowledged && restored.size() >= 33;
            for (size_t i = 1; same && i < original.size(); ++i)
                same = i == 4 ? ((restored[i] & 7) == (original[i] & 7)) : restored[i] == original[i];
            passed = passed && same;
            consoleOut() << (same ? "[IM948 RESTORE PASS] Original parameters verified" :
                "[IM948 RESTORE FAIL] Original parameters could not be verified") << std::endl;
        };
        consoleOut() << "[IM948 TEMP] Subscription mask=0x0046; other parameters preserved" << std::endl;
        const bool acknowledged = !query(0x12, 1, temporary).empty();
        auto configured = acknowledged ? query(0x10, 33) : std::vector<uint8_t>{};
        passed = !configured.empty() && configured[9] == 0x46 && configured[10] == 0;
    }
    auto sample = passed ? query(0x11, 7) : std::vector<uint8_t>{};
    passed = passed && !sample.empty();
    if (!sample.empty()) {
        const unsigned mask = unsigned(sample[1]) | (unsigned(sample[2]) << 8);
        const uint32_t timestamp = uint32_t(sample[3]) | (uint32_t(sample[4]) << 8) |
            (uint32_t(sample[5]) << 16) | (uint32_t(sample[6]) << 24);
        consoleOut() << "[IM948 SAMPLE] timestamp=" << timestamp << "ms subscription_mask=" << mask
            << " payload_bytes=" << sample.size() - 7 << std::endl;
        if (!mask) consoleOut() << "[IM948 INFO] Device has no enabled sensor fields; configuration preserved." << std::endl;
        if (sensorFields && (mask != 0x46 || sample.size() != 25)) passed = false;
        if (mask == 0x46 && sample.size() == 25) {
            auto signed16 = [&](size_t offset) {
                const int value = int(sample[offset]) | (int(sample[offset + 1]) << 8);
                return value >= 32768 ? value - 65536 : value;
            };
            auto vector = [&](size_t offset, double scale) {
                std::ostringstream out; out << std::fixed << std::setprecision(4);
                for (int axis = 0; axis < 3; ++axis) out << (axis ? ", " : "") << signed16(offset + axis * 2) * scale;
                return out.str();
            };
            consoleOut() << "[IM948 VALUES] accel(m/s^2)=[" << vector(7, 0.00478515625)
                << "] gyro(deg/s)=[" << vector(13, 0.06103515625)
                << "] Euler(deg)=[" << vector(19, 0.0054931640625) << "]" << std::endl;
        }
    }
    // Prove that subsequent queries still receive data on the same connection.
    for (int i = 0; passed && i < 2; ++i) {
        std::this_thread::sleep_for(std::chrono::milliseconds(250));
        passed = !query(0x11, 7).empty();
    }
    if (restore.action) { auto action = std::move(restore.action); action(); }
    central.peripheral_Unsubscribe(service, rx);
    if (central.peripheral_Is_Connected()) central.peripheral_Disconnect();
    passed = passed && !capture->error && !central.peripheral_Is_Connected();
    consoleOut() << (passed ? "[IM948 PASS] Keepalive, status, sample requests/data, parameter restoration (if requested), unsubscribe and disconnect" :
        "[IM948 FAIL] Read session incomplete") << std::endl;
    return passed;
}
