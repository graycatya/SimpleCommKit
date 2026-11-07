"""Build the experimental Kotlin Android BLE AAR without Gradle dependencies."""
import argparse
from pathlib import Path
import subprocess
import zipfile
import os
import re

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--sdk', type=Path, required=True)
parser.add_argument('--ndk', type=Path, required=True)
parser.add_argument('--java-home', type=Path, required=True)
parser.add_argument('--kotlin-home', type=Path, required=True, help='Kotlin compiler distribution (contains lib/kotlin-compiler.jar)')
parser.add_argument('--compile-sdk', default='35')
parser.add_argument('--abi', action='append', choices=['arm64-v8a', 'armeabi-v7a', 'x86_64', 'x86'])
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
out = root / 'build' / 'android-ble-aar'
out.mkdir(parents=True, exist_ok=True)
bridge = root / 'src' / 'SimpleCommKitBle' / 'android'
jar = args.sdk / 'platforms' / ('android-' + args.compile_sdk) / 'android.jar'
if not jar.is_file():
    parser.error('compile SDK android.jar not found')

def run(*command):
    subprocess.run([str(x) for x in command], cwd=root, check=True)

classes_jar = out / 'classes.jar'
compiler = args.kotlin_home / 'lib' / 'kotlin-compiler.jar'
stdlib = args.kotlin_home / 'lib' / 'kotlin-stdlib.jar'
if not compiler.is_file() or not stdlib.is_file():
    parser.error('Kotlin compiler distribution or stdlib not found')
# Compile directly into a fresh jar: never package stale Java classes.
run(args.java_home / 'bin' / 'java', '-Dkotlin.home=' + str(args.kotlin_home),
    '-cp', str(args.kotlin_home / 'lib' / '*'), 'org.jetbrains.kotlin.cli.jvm.K2JVMCompiler',
    '-jvm-target', '1.8', '-no-reflect', '-no-stdlib',
    '-classpath', os.pathsep.join([str(jar), str(stdlib)]),
    '-d', classes_jar, bridge / 'kotlin/org/simplecommkit/ble/NativeBle.kt')
abi = subprocess.check_output([str(args.java_home / 'bin' / 'javap'), '-p', '-s',
    '-classpath', str(classes_jar), 'org.simplecommkit.ble.NativeBle'], text=True)
(out / 'kotlin-jni-signatures.txt').write_text(abi, encoding='utf-8')
# Companion external functions must remain static natives on the outer class.
expected = {
    'nativeInitialize': '()V',
    'nativeAdvertisement': '(JJLjava/lang/String;Ljava/lang/String;I[I[[B)V',
    'nativeScanFailed': '(JJI)V',
    'nativeCancelled': '(J)Z',
    'nativeGattEvent': '(JJILjava/lang/String;Ljava/lang/String;[B)V',
}
for name, descriptor in expected.items():
    if not re.search(r'static[^\n]*native[^\n]*\b' + name +
                     r'\([^\n]*\);\s+descriptor: ' + re.escape(descriptor), abi):
        raise RuntimeError('Kotlin JNI ABI mismatch: ' + name)
for method, descriptor in [('available', '()Z'), ('enabled', '()Z'),
                           ('startScan', '(J)V'), ('stopScan', '()V'), ('close', '()V'),
                           ('transact', '(ILjava/lang/String;Ljava/lang/String;Ljava/lang/String;[BIJJ)[B')]:
    if not re.search(r'public[^\n]*\b' + method + r'\([^\n]*\);\s+descriptor: ' + re.escape(descriptor), abi):
        raise RuntimeError('Kotlin bridge ABI mismatch: ' + method)
libraries = []
for abi in args.abi or ['arm64-v8a', 'x86_64']:
    build = out / abi
    run('cmake', '-S', root, '-B', build, '-G', 'Ninja',
        '-DCMAKE_TOOLCHAIN_FILE=' + str(args.ndk / 'build/cmake/android.toolchain.cmake'),
        '-DANDROID_ABI=' + abi, '-DANDROID_PLATFORM=android-23', '-DANDROID_STL=c++_shared',
        '-DCMAKE_BUILD_TYPE=Debug', '-DENABLE_SIMPLECOMMKIT_BLE=ON',
        '-DSIMPLECOMMKIT_BUILD_SHARED_LIBS=ON',
        '-DENABLE_SIMPLECOMMKIT_TCP=OFF', '-DENABLE_SIMPLECOMMKIT_UDP=OFF',
        '-DENABLE_SIMPLECOMMKIT_WEBSOCKET=OFF')
    run('cmake', '--build', build, '--parallel', '4')
    triples = {'arm64-v8a': 'aarch64-linux-android', 'armeabi-v7a': 'arm-linux-androideabi',
               'x86_64': 'x86_64-linux-android', 'x86': 'i686-linux-android'}
    runtimes = list((args.ndk / 'toolchains/llvm/prebuilt').glob(
        '*/sysroot/usr/lib/' + triples[abi] + '/libc++_shared.so'))
    if len(runtimes) != 1:
        raise RuntimeError('Cannot uniquely locate libc++_shared.so for ' + abi)
    for source in [build / 'src/SimpleCommKitBle/libSimpleCommKitBle.so',
                   build / 'src/SimpleCommKitUtil/libSimpleCommKitUtil.so', runtimes[0]]:
        libraries.append((source, 'jni/' + abi + '/' + source.name))

aar = out / 'SimpleCommKitBle-debug.aar'
with zipfile.ZipFile(aar, 'w', compression=zipfile.ZIP_DEFLATED) as archive:
    archive.write(bridge / 'AndroidManifest.xml', 'AndroidManifest.xml')
    archive.write(bridge / 'consumer-rules.pro', 'proguard.txt')
    archive.write(classes_jar, 'classes.jar')
    # The Kotlin host supplies stdlib; embedding it would create duplicate classes.
    archive.writestr('R.txt', '')
    archive.write(root / 'LICENSE', 'META-INF/SimpleCommKit-LICENSE.txt')
    for notice in ['NOTICE', 'NOTICE.toolchain']:
        archive.write(args.ndk / notice, 'META-INF/android-ndk-' + notice)
    for source, target in libraries:
        archive.write(source, target)
print('Experimental Kotlin BLE AAR:', aar)
