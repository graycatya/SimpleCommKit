# IM948 unified probe

From this example's Windows build directory:

```powershell
.\SimpleCommKitBleCentralExample.exe --scan-seconds 25 --device-address CC:A7:75:0D:58:F0 --im948-test
```

Select the Bluetooth adapter when prompted. `--im948-test` performs two
sessions through the public API, reconnecting for the second session:

- Check AE30/AE10 Read and AE30/AE05 Indicate capabilities.
- Read AE10 and print the returned byte count.
- Subscribe/unsubscribe AE05; print indication count and check callbacks stop.
  Successful subscription does not prove indication payload delivery without
  a documented device trigger.
- Run the sensor probe through AE01 Write Command and AE02 Notify, verify
  the initial sensor sample, restore and verify original parameters, then
  unsubscribe and disconnect.

The final result is `[IM948 TEST PASS]` or `[IM948 TEST FAIL]`, with matching
process exit status. This short two-session test does not establish long-term
stability. Undocumented AE10 Write Request operations are explicitly skipped.
Temporary sensor configuration restoration requires the device to remain
connected; forced termination can prevent restoration.

Existing `--im948-read` and `--im948-sensor-read` remain available for a single
protocol session.
