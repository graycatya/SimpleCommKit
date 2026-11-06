package org.simplecommkit.ble.probe

import android.app.Activity
import android.os.Bundle
import android.util.Log
import android.widget.TextView
import android.widget.ScrollView
import java.util.concurrent.atomic.AtomicBoolean
import org.simplecommkit.ble.NativeBle

/** Foreground C++ integration harness. IM948 mode is selected by an explicit address extra. */
class MainActivity : Activity() {
    companion object {
        private val running = AtomicBoolean(false)
        @JvmStatic private external fun nativeUninitialized(): String
        @JvmStatic private external fun nativeScan(): String
        @JvmStatic private external fun nativeIm948(address: String): String
    }
    override fun onCreate(state: Bundle?) {
        super.onCreate(state)
        val text = TextView(this)
        text.text = "Running Kotlin / JNI / C++ BLE probe…"
        setContentView(ScrollView(this).apply { addView(text) })
        if (!running.compareAndSet(false, true)) {
            text.text = "A BLE test is already running. Wait for it to finish."
            return
        }
        Thread {
            val result = try {
                System.loadLibrary("SimpleCommKitUtil")
                System.loadLibrary("SimpleCommKitBle")
                System.loadLibrary("BleProbe")
                val initialization = nativeUninitialized()
                NativeBle.initialize(applicationContext)
                val address = intent.getStringExtra("device_address")
                initialization + "\n" + if (address == null) nativeScan() else nativeIm948(address)
            } catch (failure: Throwable) { "FAIL: $failure" }
            Log.i("SCKBleProbe", result)
            openFileOutput("result.txt", MODE_PRIVATE).use { it.write(result.toByteArray()) }
            runOnUiThread { text.text = result }
            running.set(false)
        }.start()
    }
}
