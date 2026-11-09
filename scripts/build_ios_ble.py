#!/usr/bin/env python3
"""Build the iOS CoreBluetooth library and optional foreground probe App."""
import argparse
from pathlib import Path
import shlex
import subprocess
import sys


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--platform', choices=('device', 'simulator'), default='simulator')
    parser.add_argument('--arch', choices=('arm64', 'x86_64'), default='arm64')
    parser.add_argument('--deployment-target', default='15.0')
    parser.add_argument('--config', choices=('Debug', 'Release'), default='Debug')
    parser.add_argument('--build-dir', type=Path)
    parser.add_argument('--team', default='', help='Existing Apple development team ID (device signing)')
    parser.add_argument('--profile', default='', help='Optional manually managed provisioning profile UUID/name')
    parser.add_argument('--library-only', action='store_true')
    parser.add_argument('--shared', action='store_true', help='Build a dynamic library without the probe App')
    parser.add_argument('--dry-run', action='store_true')
    args = parser.parse_args()
    if sys.platform != 'darwin':
        parser.error('iOS builds require macOS with Xcode')
    if args.platform == 'device' and args.arch != 'arm64':
        parser.error('the device target requires arm64')
    if args.shared and not args.library_only:
        parser.error('--shared requires --library-only; the probe App links static libraries')
    if args.profile and not args.team:
        parser.error('--profile requires --team')
    if args.platform == 'simulator' and (args.team or args.profile):
        parser.error('simulator builds do not require a development team/profile')
    try:
        version = tuple(map(int, args.deployment_target.split('.')))
        if version < (15, 0):
            raise ValueError()
    except ValueError:
        parser.error('--deployment-target must be an iOS version >= 15.0')
    root = Path(__file__).resolve().parents[1]
    build = (args.build_dir or root / 'build' / ('ios-ble-' + args.platform)).resolve()
    signed = args.platform == 'device' and bool(args.team) and not args.library_only
    configure = ['cmake', '-S', str(root), '-B', str(build), '-G', 'Xcode',
                 '-DCMAKE_SYSTEM_NAME=iOS',
                 '-DCMAKE_OSX_SYSROOT=' + ('iphoneos' if args.platform == 'device' else 'iphonesimulator'),
                 '-DCMAKE_OSX_ARCHITECTURES=' + args.arch,
                 '-DCMAKE_OSX_DEPLOYMENT_TARGET=' + args.deployment_target,
                 '-DCMAKE_XCODE_ATTRIBUTE_CODE_SIGNING_ALLOWED=' + ('YES' if signed else 'NO'),
                 '-DCMAKE_XCODE_ATTRIBUTE_DEVELOPMENT_TEAM=' + args.team,
                 '-DSIMPLECOMMKIT_IOS_TEAM=' + args.team,
                 '-DSIMPLECOMMKIT_IOS_PROFILE=' + args.profile,
                 '-DSIMPLECOMMKIT_IOS_BLE_PROBE=' + ('OFF' if args.library_only else 'ON'),
                 '-DSIMPLECOMMKIT_BUILD_SHARED_LIBS=' + ('ON' if args.shared else 'OFF'),
                 '-DENABLE_SIMPLECOMMKIT_BLE=ON', '-DENABLE_SIMPLECOMMKIT_TCP=OFF',
                 '-DENABLE_SIMPLECOMMKIT_UDP=OFF', '-DENABLE_SIMPLECOMMKIT_WEBSOCKET=OFF',
                 '-DENABLE_SIMPLECOMMKIT_HID=OFF', '-DENABLE_SIMPLECOMMKIT_USB=OFF',
                 '-DENABLE_SIMPLECOMMKIT_SERIALPORT=OFF', '-DENABLE_SIMPLECOMMKIT_MQTTCLIENT=OFF',
                 '-DSIMPLECOMMKIT_EXAMPLES=OFF', '-DSIMPLECOMMKIT_BLE_TESTS=OFF',
                 '-DENABLE_SIMPLECOMMKITPYBIND=OFF', '-DENABLE_SIMPLECOMMKITAI_FASTMCPP=OFF']
    compile_command = ['cmake', '--build', str(build), '--config', args.config,
                       '--target', 'SimpleCommKitBle' if args.library_only else 'SimpleCommKitBleIosExample', '-j', '4']
    for command in (configure, compile_command):
        print(shlex.join(command), flush=True)
        if not args.dry_run:
            subprocess.run(command, check=True)
    if not args.library_only:
        suffix = 'iphoneos' if args.platform == 'device' else 'iphonesimulator'
        print('App:', build / 'examples/SimpleCommKitBle/SimpleCommKitBleIosExample' /
              (args.config + '-' + suffix) / 'SimpleCommKitBleIosExample.app')
        if args.platform == 'device' and not signed:
            print('Unsigned device App: rebuild with --team before installing on a physical device.')


if __name__ == '__main__':
    main()
