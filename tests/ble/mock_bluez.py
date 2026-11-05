"""Run a native C++ consumer against a private D-Bus BlueZ service (no hardware)."""
import os
import subprocess
import sys
import dbus
import dbus.service
from dbus.mainloop.glib import DBusGMainLoop
from gi.repository import GLib

DBusGMainLoop(set_as_default=True)
os.environ['DBUS_SYSTEM_BUS_ADDRESS'] = os.environ['DBUS_SESSION_BUS_ADDRESS']
bus = dbus.SessionBus()
name = dbus.service.BusName('org.bluez', bus)
ADAPTER = 'org.bluez.Adapter1'
DEVICE = 'org.bluez.Device1'
CHAR = 'org.bluez.GattCharacteristic1'
DESC = 'org.bluez.GattDescriptor1'
SERVICE = 'org.bluez.GattService1'
PROPS = 'org.freedesktop.DBus.Properties'
a = '/org/bluez/hci0'
d = a + '/dev_00_11_22_33_44_55'
s = d + '/service001'
c = s + '/char001'
x = c + '/desc001'
objects = {}

def uuid(short):
    return '0000' + short + '-0000-1000-8000-00805f9b34fb'

def values(data):
    return dbus.Array(data, signature='y')

class Object(dbus.service.Object):
    def __init__(self, path, interface, props):
        super().__init__(bus, path)
        self.path, self.interface, self.props = path, interface, props
        objects[path] = self

    @dbus.service.signal(PROPS, signature='sa{sv}as')
    def PropertiesChanged(self, interface, changed, invalidated):
        pass

    def update(self, **props):
        self.props.update(props)
        self.PropertiesChanged(self.interface, props, [])

    @dbus.service.method(PROPS, in_signature='ssv', out_signature='')
    def Set(self, interface, prop, value):
        assert interface == self.interface
        self.update(**{prop: value})

class Manager(dbus.service.Object):
    @dbus.service.method('org.freedesktop.DBus.ObjectManager', out_signature='a{oa{sa{sv}}}')
    def GetManagedObjects(self):
        return {path: {obj.interface: obj.props} for path, obj in objects.items()}

class Adapter(Object):
    @dbus.service.method(ADAPTER, in_signature='a{sv}', out_signature='')
    def SetDiscoveryFilter(self, options):
        assert options['Transport'] == 'le'

    @dbus.service.method(ADAPTER)
    def StartDiscovery(self):
        objects[d] = device_object
        self.update(Discovering=dbus.Boolean(True))
        GLib.timeout_add(70, self.advertise)

    def advertise(self):
        if not self.props['Discovering']:
            return False
        # Intentionally identical RSSI/data: every advertisement still updates.
        objects[d].update(RSSI=dbus.Int16(-42))
        return True

    @dbus.service.method(ADAPTER)
    def StopDiscovery(self):
        self.update(Discovering=dbus.Boolean(False))
        if os.environ.get('BLE_MOCK_TRANSIENT_DEVICE') and not device_object.props['Connected']:
            objects.pop(d, None)

    @dbus.service.method(ADAPTER, in_signature='o')
    def RemoveDevice(self, path):
        del objects[str(path)]

class Device(Object):
    @dbus.service.method(DEVICE)
    def Connect(self):
        self.update(Connected=dbus.Boolean(True), ServicesResolved=dbus.Boolean(True))

    @dbus.service.method(DEVICE)
    def Disconnect(self):
        self.update(Connected=dbus.Boolean(False), ServicesResolved=dbus.Boolean(False))

class NotAuthorized(dbus.DBusException):
    _dbus_error_name = 'org.bluez.Error.NotAuthorized'

class Characteristic(Object):
    @dbus.service.method(CHAR, in_signature='a{sv}', out_signature='ay')
    def ReadValue(self, options):
        return self.props['Value']

    @dbus.service.method(CHAR, in_signature='aya{sv}')
    def WriteValue(self, data, options):
        assert options['type'] in ('request', 'command')
        if list(data) == [238]:
            raise NotAuthorized('mock permission denied')
        if list(data) == [221]:
            Object(s + '_duplicate', SERVICE, dict(Device=dbus.ObjectPath(d), UUID=uuid('180d')))
        self.props['Value'] = data

    @dbus.service.method(CHAR)
    def StartNotify(self):
        self.props['Notifying'] = dbus.Boolean(True)
        GLib.timeout_add(30, self.notify)

    def notify(self):
        if not self.props['Notifying']:
            return False
        self.PropertiesChanged(CHAR, {'Value': self.props['Value']}, [])
        return True

    @dbus.service.method(CHAR)
    def StopNotify(self):
        self.props['Notifying'] = dbus.Boolean(False)

class Descriptor(Object):
    @dbus.service.method(DESC, in_signature='a{sv}', out_signature='ay')
    def ReadValue(self, options):
        return self.props['Value']

    @dbus.service.method(DESC, in_signature='aya{sv}')
    def WriteValue(self, data, options):
        self.props['Value'] = data

manager = Manager(bus, '/')
Adapter(a, ADAPTER, dict(Address='00:00:00:00:00:01', Powered=dbus.Boolean(True), Discovering=dbus.Boolean(False)))
device_object = Device(d, DEVICE, dict(Adapter=dbus.ObjectPath(a), Address='00:11:22:33:44:55', AddressType='public',
    Name='Mock BLE', RSSI=dbus.Int16(-42), Connected=dbus.Boolean(False), ServicesResolved=dbus.Boolean(False),
    Paired=dbus.Boolean(True), TxPower=dbus.Int16(-9),
    ServiceData=dbus.Dictionary({uuid('180d'): values([9, 0])}, signature='sv'), ManufacturerData=dbus.Dictionary({1: values([0, 255])}, signature='qv')))
Object(s, SERVICE, dict(Device=dbus.ObjectPath(d), UUID=uuid('180d')))
Characteristic(c, CHAR, dict(Service=dbus.ObjectPath(s), UUID=uuid('2a37'),
    Flags=dbus.Array(['read', 'write', 'write-without-response', 'notify'], signature='s'),
    Value=values([0, 1, 255]), Notifying=dbus.Boolean(False)))
Descriptor(x, DESC, dict(Characteristic=dbus.ObjectPath(c), UUID=uuid('2901'), Value=values([0, 1, 255])))
process = subprocess.Popen(sys.argv[1:])
loop = GLib.MainLoop()
def poll():
    if process.poll() is not None:
        loop.quit()
        return False
    return True
GLib.timeout_add(20, poll)
loop.run()
sys.exit(process.returncode)
