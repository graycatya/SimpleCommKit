# Kotlin JVM methods are called from JNI; native symbols use this exact package name.
-keep class org.simplecommkit.ble.NativeBle { *; }
-keep class org.simplecommkit.ble.NativeBle$* { *; }
