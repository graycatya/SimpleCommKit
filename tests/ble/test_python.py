"""Exercise existing Python calls and callback lifetimes against mock BlueZ."""
import gc
import sys
import time
sys.path.insert(0, sys.argv[1])
import _SimpleCommKitPyBle as ble

def until(predicate):
    end = time.monotonic() + 3
    while not predicate():
        assert time.monotonic() < end, 'Python BLE callback timed out'
        time.sleep(0.001)

central = ble.BleCentral()
errors, found, payloads = [], [], []
central.set_callback_error(errors.append)
central.set_current_adapter(central.get_adapters()[0])
central.adapter_set_callback_on_scan_found(found.append)
central.adapter_scan_start()
until(lambda: found)
central.adapter_scan_stop()
central.set_current_peripheral(found[0])
central.peripheral_connect()
assert central.peripheral_read('180d', '2a37') == b'\x00\x01\xff'
central.peripheral_notify('180d', '2a37', payloads.append)
until(lambda: len(payloads) >= 2)
central.peripheral_unsubscribe('180d', '2a37')
count = len(payloads)
time.sleep(0.08)
assert len(payloads) == count
central.peripheral_disconnect()
del central
gc.collect()
assert not errors, errors
print('Python BLE native integration passed')

# Destruction from the event thread must not join itself or retain callbacks.
holder = {'central': ble.BleCentral()}
destroyed = []
second_found = []
holder['central'].set_current_adapter(holder['central'].get_adapters()[0])
holder['central'].adapter_set_callback_on_scan_found(second_found.append)
holder['central'].adapter_scan_start()
until(lambda: second_found)
holder['central'].adapter_scan_stop()
holder['central'].set_current_peripheral(second_found[0])
holder['central'].peripheral_connect()
def destroy_in_callback(payload):
    holder.pop('central', None)
    destroyed.append(True)
holder['central'].peripheral_notify('180d', '2a37', destroy_in_callback)
until(lambda: destroyed)
assert not holder
print('Python callback destruction passed')
