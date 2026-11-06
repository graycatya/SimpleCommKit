package org.simplecommkit.ble

import android.Manifest
import android.bluetooth.BluetoothManager
import android.bluetooth.BluetoothDevice
import android.bluetooth.BluetoothGatt
import android.bluetooth.BluetoothGattCallback
import android.bluetooth.BluetoothGattCharacteristic
import android.bluetooth.BluetoothGattDescriptor
import android.bluetooth.BluetoothProfile
import android.bluetooth.le.BluetoothLeScanner
import android.bluetooth.le.ScanCallback
import android.bluetooth.le.ScanResult
import android.content.Context
import android.content.pm.PackageManager
import android.os.Build
import android.os.SystemClock
import java.util.UUID

/** Load the native library before initialize; only Application Context is retained. */
class NativeBle(private val handle: Long) {
    companion object {
        private var application: Context? = null

        @JvmStatic
        @Synchronized
        fun initialize(context: Context) {
            application = requireNotNull(context.applicationContext) { "Application Context required" }
            nativeInitialize()
        }

        // Keep static native methods on NativeBle for the existing JNI exports.
        @JvmStatic private external fun nativeInitialize()
        @JvmStatic private external fun nativeAdvertisement(
            handle: Long, epoch: Long, address: String, name: String, rssi: Int,
            companies: IntArray, manufacturer: Array<ByteArray>
        )
        @JvmStatic private external fun nativeScanFailed(handle: Long, epoch: Long, reason: Int)
        @JvmStatic private external fun nativeCancelled(handle: Long): Boolean
        @JvmStatic private external fun nativeGattEvent(handle: Long, token: Long, type: Int,
            service: String, characteristic: String, data: ByteArray)
    }

    private val context = synchronized(NativeBle::class.java) {
        checkNotNull(application) { "Call NativeBle.initialize first" }
    }
    private val adapter = context.getSystemService(BluetoothManager::class.java)?.adapter
    private var scanner: BluetoothLeScanner? = null
    private var callback: ScanCallback? = null
    @Volatile private var closed = false
    private var epoch = 0L
    class BleFailure(val errorCode: Int, message: String) : RuntimeException(message)
    private val gattLock = java.lang.Object()
    private var gatt: BluetoothGatt? = null
    private var linkToken = 0L
    private var linked = false
    private var negotiatedMtu = 23
    private var pending = ""
    private var pendingTarget: Any? = null
    private var completed = false
    private var result = ByteArray(0)
    private var status = 0
    private val subscriptions = mutableSetOf<Pair<String, String>>()

    private fun fail(code: Int, message: String): Nothing = throw BleFailure(65536 + code, message)
    private fun closeGatt() {
        val old = gatt
        gatt = null; linked = false; subscriptions.clear()
        completed = true; status = -1; gattLock.notifyAll()
        if (old != null) try { old.disconnect() } finally { old.close() }
    }
    private fun begin(kind: String, target: Any? = null) {
        pending = kind; pendingTarget = target; completed = false; status = 0; result = ByteArray(0)
    }
    private fun finish(kind: String, target: Any?, code: Int, bytes: ByteArray = ByteArray(0)) {
        if (pending != kind || (pendingTarget != null && pendingTarget !== target)) return
        status = code; result = bytes.clone(); completed = true; gattLock.notifyAll()
    }
    private fun await(end: Long): ByteArray {
        while (!completed) {
            if (nativeCancelled(handle)) { closeGatt(); fail(19, "GATT operation cancelled; reconnect required") }
            val remaining = end - SystemClock.elapsedRealtime()
            if (remaining <= 0) { closeGatt(); fail(17, "GATT operation timed out; reconnect required") }
            gattLock.wait(minOf(remaining, 20L))
        }
        pending = ""; pendingTarget = null
        if (status != BluetoothGatt.GATT_SUCCESS) {
            val failure = status
            closeGatt()
            fail(if (failure == 5 || failure == 8 || failure == 15) 16 else 21, "GATT status $failure")
        }
        return result
    }
    private fun findCharacteristic(service: String, characteristic: String): BluetoothGattCharacteristic {
        val services = gatt!!.services.filter { it.uuid == UUID.fromString(service) }
        if (services.size > 1) fail(22, "Ambiguous service UUID")
        val matches = services.singleOrNull()?.characteristics?.filter { it.uuid == UUID.fromString(characteristic) }.orEmpty()
        if (matches.size > 1) fail(22, "Ambiguous characteristic UUID")
        return matches.singleOrNull() ?: fail(18, "Characteristic not found")
    }
    private fun findDescriptor(c: BluetoothGattCharacteristic, uuid: String): BluetoothGattDescriptor {
        val matches = c.descriptors.filter { it.uuid == UUID.fromString(uuid) }
        if (matches.size > 1) fail(22, "Ambiguous descriptor UUID")
        return matches.singleOrNull() ?: fail(18, "Descriptor not found")
    }
    private fun callback(token: Long) = object : BluetoothGattCallback() {
        override fun onConnectionStateChange(current: BluetoothGatt, code: Int, newState: Int) {
            synchronized(gattLock) {
                if (gatt !== current || linkToken != token) return
                if (newState == BluetoothProfile.STATE_CONNECTED && code == BluetoothGatt.GATT_SUCCESS) {
                    linked = true; finish("connect", null, code)
                } else if (newState == BluetoothProfile.STATE_DISCONNECTED || code != BluetoothGatt.GATT_SUCCESS) {
                    val wasLinked = linked
                    closeGatt()
                    if (wasLinked) nativeGattEvent(handle, token, 0, "", "", ByteArray(0))
                }
            }
        }
        override fun onServicesDiscovered(current: BluetoothGatt, code: Int) {
            synchronized(gattLock) { if (gatt === current) finish("services", null, code) }
        }
        override fun onMtuChanged(current: BluetoothGatt, mtu: Int, code: Int) {
            synchronized(gattLock) {
                if (gatt !== current) return
                if (code == BluetoothGatt.GATT_SUCCESS) negotiatedMtu = mtu
                finish("mtu", null, code)
            }
        }
        @Deprecated("Legacy callback for Android <33")
        override fun onCharacteristicRead(current: BluetoothGatt, c: BluetoothGattCharacteristic, code: Int) {
            if (Build.VERSION.SDK_INT < 33) readResult(current, c, c.value ?: ByteArray(0), code)
        }
        override fun onCharacteristicRead(current: BluetoothGatt, c: BluetoothGattCharacteristic, value: ByteArray, code: Int) =
            readResult(current, c, value, code)
        private fun readResult(current: BluetoothGatt, c: BluetoothGattCharacteristic, value: ByteArray, code: Int) {
            synchronized(gattLock) { if (gatt === current) finish("read", c, code, value) }
        }
        override fun onCharacteristicWrite(current: BluetoothGatt, c: BluetoothGattCharacteristic, code: Int) {
            synchronized(gattLock) { if (gatt === current) finish("write", c, code) }
        }
        @Deprecated("Legacy callback for Android <33")
        override fun onDescriptorRead(current: BluetoothGatt, d: BluetoothGattDescriptor, code: Int) {
            if (Build.VERSION.SDK_INT < 33) descriptorResult(current, d, d.value ?: ByteArray(0), code)
        }
        override fun onDescriptorRead(current: BluetoothGatt, d: BluetoothGattDescriptor, code: Int, value: ByteArray) =
            descriptorResult(current, d, value, code)
        private fun descriptorResult(current: BluetoothGatt, d: BluetoothGattDescriptor, value: ByteArray, code: Int) {
            synchronized(gattLock) { if (gatt === current) finish("readDescriptor", d, code, value) }
        }
        override fun onDescriptorWrite(current: BluetoothGatt, d: BluetoothGattDescriptor, code: Int) {
            synchronized(gattLock) { if (gatt === current) finish("writeDescriptor", d, code) }
        }
        @Deprecated("Legacy callback for Android <33")
        override fun onCharacteristicChanged(current: BluetoothGatt, c: BluetoothGattCharacteristic) {
            if (Build.VERSION.SDK_INT < 33) notification(current, c, c.value ?: ByteArray(0))
        }
        override fun onCharacteristicChanged(current: BluetoothGatt, c: BluetoothGattCharacteristic, value: ByteArray) =
            notification(current, c, value)
        private fun notification(current: BluetoothGatt, c: BluetoothGattCharacteristic, value: ByteArray) {
            synchronized(gattLock) {
                val key = c.service.uuid.toString() to c.uuid.toString()
                if (gatt === current && linked && key in subscriptions)
                    nativeGattEvent(handle, token, 1, key.first, key.second, value.clone())
            }
        }
        override fun onServiceChanged(current: BluetoothGatt) {
            synchronized(gattLock) {
                if (gatt !== current) return
                closeGatt(); nativeGattEvent(handle, token, 2, "", "", ByteArray(0))
            }
        }
    }

    /** Called serially by the C++ executor, never by the UI thread. */
    @Suppress("DEPRECATION")
    fun transact(action: Int, service: String, characteristic: String, descriptor: String,
                 payload: ByteArray, mode: Int, timeout: Long, token: Long): ByteArray = synchronized(gattLock) {
        val end = SystemClock.elapsedRealtime() + timeout.coerceAtLeast(1)
        if (action == 2) { closeGatt(); return@synchronized ByteArray(0) }
        connectPermission()
        if (closed) fail(19, "Backend closed")
        if (action == 1) {
            closeGatt(); linkToken = token; negotiatedMtu = 23
            begin("connect")
            val device = adapter?.getRemoteDevice(service) ?: fail(23, "Adapter unavailable")
            gatt = device.connectGatt(context, false, callback(token), BluetoothDevice.TRANSPORT_LE)
            if (gatt == null) fail(23, "connectGatt returned null")
            await(end)
            begin("mtu")
            if (gatt!!.requestMtu(247)) await(end) else pending = ""
            return@synchronized ByteArray(0)
        }
        val current = gatt ?: fail(23, "No GATT connection")
        if (!linked) fail(23, "GATT disconnected")
        if (action == 8) return@synchronized byteArrayOf(negotiatedMtu.toByte(), (negotiatedMtu shr 8).toByte())
        if (action == 3) {
            begin("services")
            if (!current.discoverServices()) fail(21, "discoverServices rejected")
            await(end)
            return@synchronized buildString {
                for (s in current.services) {
                    append("S ").append(s.uuid).append('\n')
                    for (c in s.characteristics) {
                        append("C ").append(c.uuid).append(' ').append(c.properties).append('\n')
                        for (d in c.descriptors) append("D ").append(d.uuid).append('\n')
                    }
                }
            }.toByteArray(Charsets.UTF_8)
        }
        val c = findCharacteristic(service, characteristic)
        val d = if (descriptor.isEmpty()) null else findDescriptor(c, descriptor)
        if (action == 4) {
            if (d == null && c.properties and BluetoothGattCharacteristic.PROPERTY_READ == 0) fail(15, "Read unsupported")
            begin(if (d == null) "read" else "readDescriptor", d ?: c)
            val accepted = if (d == null) current.readCharacteristic(c) else current.readDescriptor(d)
            if (!accepted) fail(21, "Read rejected")
            return@synchronized await(end)
        }
        fun writeDescriptor(target: BluetoothGattDescriptor, value: ByteArray) {
            begin("writeDescriptor", target)
            val accepted = if (Build.VERSION.SDK_INT >= 33) current.writeDescriptor(target, value) == 0
                else { target.value = value; current.writeDescriptor(target) }
            if (!accepted) fail(21, "Descriptor write rejected")
            await(end)
        }
        val cccd = "00002902-0000-1000-8000-00805f9b34fb"
        if (action == 5) {
            if (d != null) {
                if (descriptor == cccd) fail(15, "Use subscribe for CCCD")
                writeDescriptor(d, payload)
            } else {
                val property = if (mode == 1) BluetoothGattCharacteristic.PROPERTY_WRITE_NO_RESPONSE else BluetoothGattCharacteristic.PROPERTY_WRITE
                if (c.properties and property == 0) fail(15, "Write mode unsupported")
                if (payload.size > negotiatedMtu - 3) fail(18, "Payload exceeds negotiated MTU")
                val writeType = if (mode == 1) BluetoothGattCharacteristic.WRITE_TYPE_NO_RESPONSE else BluetoothGattCharacteristic.WRITE_TYPE_DEFAULT
                begin("write", c)
                val accepted = if (Build.VERSION.SDK_INT >= 33) current.writeCharacteristic(c, payload, writeType) == 0
                    else { c.writeType = writeType; c.value = payload; current.writeCharacteristic(c) }
                if (!accepted) fail(21, "Characteristic write rejected")
                await(end)
            }
            return@synchronized ByteArray(0)
        }
        if (action == 6 || action == 7) {
            val key = service to characteristic
            val enable = action == 6
            if (enable) {
                val property = if (mode == 1) BluetoothGattCharacteristic.PROPERTY_INDICATE else BluetoothGattCharacteristic.PROPERTY_NOTIFY
                if (c.properties and property == 0) fail(15, "Subscription mode unsupported")
            } else if (key !in subscriptions) return@synchronized ByteArray(0)
            val target = findDescriptor(c, cccd)
            if (!enable) subscriptions.remove(key)
            if (!current.setCharacteristicNotification(c, enable)) fail(21, "Local notification registration rejected")
            if (enable) subscriptions.add(key)
            try {
                writeDescriptor(target, if (!enable) BluetoothGattDescriptor.DISABLE_NOTIFICATION_VALUE
                    else if (mode == 1) BluetoothGattDescriptor.ENABLE_INDICATION_VALUE else BluetoothGattDescriptor.ENABLE_NOTIFICATION_VALUE)
            } catch (failure: RuntimeException) { closeGatt(); throw failure }
            return@synchronized ByteArray(0)
        }
        fail(18, "Unknown GATT action")
    }

    private fun permission(name: String) {
        if (context.checkSelfPermission(name) != PackageManager.PERMISSION_GRANTED)
            throw SecurityException("Missing runtime permission: $name")
    }
    private fun connectPermission() {
        if (Build.VERSION.SDK_INT >= 31) permission(Manifest.permission.BLUETOOTH_CONNECT)
    }
    fun available(): Boolean = adapter != null &&
        context.packageManager.hasSystemFeature(PackageManager.FEATURE_BLUETOOTH_LE)

    fun enabled(): Boolean {
        connectPermission()
        return adapter?.isEnabled == true
    }

    @Synchronized
    fun startScan(scanEpoch: Long) {
        check(!closed) { "BLE bridge closed" }
        if (callback != null) return
        connectPermission()
        permission(if (Build.VERSION.SDK_INT >= 31) Manifest.permission.BLUETOOTH_SCAN
                   else Manifest.permission.ACCESS_FINE_LOCATION)
        check(enabled()) { "Bluetooth unavailable or disabled" }
        val activeScanner = checkNotNull(adapter?.bluetoothLeScanner) { "BLE scanner unavailable" }
        scanner = activeScanner
        epoch = scanEpoch
        val activeCallback = object : ScanCallback() {
            override fun onScanResult(type: Int, result: ScanResult) {
                synchronized(this@NativeBle) {
                    if (closed || callback !== this || epoch != scanEpoch) return
                    val record = result.scanRecord
                    val data = record?.manufacturerSpecificData
                    val count = data?.size() ?: 0
                    val companies = IntArray(count) { data!!.keyAt(it) }
                    val bytes = Array(count) { data!!.valueAt(it).clone() }
                    try {
                        nativeAdvertisement(handle, scanEpoch, result.device.address,
                            record?.deviceName.orEmpty(), result.rssi, companies, bytes)
                    } catch (_: SecurityException) {
                        try { stopScan() } catch (_: RuntimeException) { }
                        nativeScanFailed(handle, scanEpoch, -1)
                    }
                }
            }
            override fun onScanFailed(reason: Int) {
                synchronized(this@NativeBle) {
                    if (closed || callback !== this || epoch != scanEpoch) return
                    callback = null
                    nativeScanFailed(handle, scanEpoch, reason)
                }
            }
        }
        callback = activeCallback
        try { activeScanner.startScan(activeCallback) }
        catch (failure: RuntimeException) { callback = null; throw failure }
    }

    @Synchronized
    fun stopScan() {
        val old = callback
        callback = null
        ++epoch
        if (old != null) scanner?.stopScan(old)
    }

    @Synchronized
    fun close() {
        closed = true
        try { stopScan() } catch (_: RuntimeException) { /* callbacks already revoked */ }
        synchronized(gattLock) { closeGatt() }
    }
}
