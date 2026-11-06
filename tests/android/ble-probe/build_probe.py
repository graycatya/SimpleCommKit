"""Build a Kotlin foreground BLE/IM948 test APK using local SDK/NDK tools."""
import argparse
import os
from pathlib import Path
import subprocess
import sys
import zipfile

p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--sdk', type=Path, required=True)
p.add_argument('--ndk', type=Path, required=True)
p.add_argument('--java-home', type=Path, required=True)
p.add_argument('--kotlin-home', type=Path, required=True)
a = p.parse_args()
source = Path(__file__).resolve().parent
root = source.parents[2]
out = root / 'build/android-ble-probe'
out.mkdir(parents=True, exist_ok=True)
tools = a.sdk / 'build-tools/37.0.0'
android = a.sdk / 'platforms/android-35/android.jar'
java = a.java_home / 'bin/java'

def run(*args):
    subprocess.run([str(v) for v in args], cwd=root, check=True)

run(sys.executable, root / 'scripts/build_android_ble.py', '--sdk', a.sdk,
    '--ndk', a.ndk, '--java-home', a.java_home, '--kotlin-home', a.kotlin_home, '--abi', 'arm64-v8a')
aar = root / 'build/android-ble-aar'
native = aar / 'arm64-v8a'
run('cmake', '-S', source, '-B', out / 'native', '-G', 'Ninja',
    '-DCMAKE_TOOLCHAIN_FILE=' + str(a.ndk / 'build/cmake/android.toolchain.cmake'),
    '-DANDROID_ABI=arm64-v8a', '-DANDROID_PLATFORM=android-23', '-DANDROID_STL=c++_shared',
    '-DBLE_LIB_DIR=' + str(native / 'src/SimpleCommKitBle'),
    '-DUTIL_LIB_DIR=' + str(native / 'src/SimpleCommKitUtil'))
run('cmake', '--build', out / 'native', '--parallel', '4')
stdlib = a.kotlin_home / 'lib/kotlin-stdlib.jar'
activity = out / 'activity.jar'
run(java, '-Dkotlin.home=' + str(a.kotlin_home), '-cp', a.kotlin_home / 'lib/*',
    'org.jetbrains.kotlin.cli.jvm.K2JVMCompiler', '-jvm-target', '1.8', '-no-stdlib', '-no-reflect',
    '-classpath', os.pathsep.join(map(str, [android, stdlib, aar / 'classes.jar'])),
    '-d', activity, source / 'MainActivity.kt')
run(java, '-cp', tools / 'lib/d8.jar', 'com.android.tools.r8.D8', '--min-api', '23',
    '--lib', android, '--output', out, activity, aar / 'classes.jar', stdlib)
unsigned = out / 'unsigned.apk'
run(tools / 'aapt.exe', 'package', '-f', '-M', source / 'AndroidManifest.xml',
    '-I', android, '-F', unsigned)
with zipfile.ZipFile(unsigned, 'a') as apk:
    apk.write(out / 'classes.dex', 'classes.dex')
    with zipfile.ZipFile(aar / 'SimpleCommKitBle-debug.aar') as archive:
        for name in archive.namelist():
            if name.startswith('jni/'):
                apk.writestr('lib/' + name[4:], archive.read(name))
    apk.write(out / 'native/libBleProbe.so', 'lib/arm64-v8a/libBleProbe.so')
aligned = out / 'aligned.apk'
run(tools / 'zipalign.exe', '-f', '-p', '4', unsigned, aligned)
key = out / 'debug.jks'
if not key.exists():
    run(a.java_home / 'bin/keytool', '-genkeypair', '-keystore', key, '-storepass', 'android',
        '-keypass', 'android', '-alias', 'androiddebugkey', '-keyalg', 'RSA', '-validity', '3650',
        '-dname', 'CN=SimpleCommKit Debug')
apk = out / 'SimpleCommKitBleProbe.apk'
run(java, '-jar', tools / 'lib/apksigner.jar', 'sign', '--ks', key, '--ks-pass', 'pass:android',
    '--key-pass', 'pass:android', '--out', apk, aligned)
run(java, '-jar', tools / 'lib/apksigner.jar', 'verify', apk)
print('Scan test APK:', apk)
