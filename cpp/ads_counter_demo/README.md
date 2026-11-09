# ADS counter demo (C++) for TwinCAT 3

C++ equivalent of `adspy1.py`. Once per second it reads `MAIN.ncounter` (LREAL) and writes an incrementing value to `MAIN.rcounter` (INT). It uses the open-source [Beckhoff ADS library](https://github.com/Beckhoff/ADS), pinned to a known commit and downloaded by CMake automatically.

Tested: built with MSVC 2022 (Build Tools) and CMake on Windows using the local TwinCAT router (TcAdsDll), and run against a TwinCAT 3 simulation on the same PC (read `MAIN.ncounter`, write `MAIN.rcounter`, clean Ctrl+C exit). The standalone-router path is only compile-tested (g++, Linux) and has not been run against a PLC.

## 1. PLC side (TwinCAT 3)

Declare these in `MAIN` (the same variables your Python script already uses):

```iecst
PROGRAM MAIN
VAR
    ncounter : LREAL;   // PLC counts up
    rcounter : INT;     // written by the PC
END_VAR

ncounter := ncounter + 1;
```

Runtime port is 851 (`AMSPORT_R0_PLC_TC3`). The PLC must be in Run mode with the program downloaded.

## 2. PC side: install tools

- Visual Studio 2022 with the "Desktop development with C++" workload (includes CMake), or standalone CMake 3.20+ and a compiler
- Git (CMake uses it to fetch the library)

## 3. Build

From a "Developer PowerShell for VS 2022":

```powershell
cd cpp/ads_counter_demo
cmake -S . -B build
cmake --build build --config Release
.\build\Release\ads_counter_demo.exe
```

Or in Visual Studio: File > Open > Folder, pick this folder, select `ads_counter_demo.exe` as the startup item, and press F5.

## 4. Run

```text
ads_counter_demo [PLC_AMS_NET_ID] [PLC_IP] [PLC_PORT] [LOCAL_AMS_NET_ID]
ads_counter_demo 10.0.0.111.1.1 10.0.0.111 851
```

Defaults match your Python script (Net ID `10.0.0.111.1.1`, port 851). The default IP `10.0.0.111` is an assumption, so pass your PLC's real IP as the second argument if it differs. Ctrl+C exits cleanly.

## 5. Which router: local TwinCAT (default on Windows) or standalone

**PLC runs on this PC (simulation / local runtime).** This is the default. CMake finds `TcAdsDll` from your TwinCAT install and uses the local TwinCAT router, the same one Python uses. No route to add. Look for `-- ADS router: TwinCAT (TcAdsDll)` in the CMake output. The IP and the 4th argument are ignored in this mode.

**PLC is a separate machine and this PC has no TwinCAT.** Configure with `-DADS_USE_TWINCAT_ROUTER=OFF` to use the library's built-in router. The PLC then needs a static route back to this PC (rest of this section).

### Standalone router: route setup

This build does not use any TwinCAT router on your PC. The PLC must have a static route back to this PC:

1. The library derives this PC's AMS Net ID from its IP: PC at `10.0.0.50` becomes `10.0.0.50.1.1`. Override it with the 4th argument if needed.
2. On the PLC, add a route in TwinCAT: SYSTEM > Routes > Static Routes > Add (or the Windows tray icon > Router > Edit Routes). Enter this PC's IP and the Net ID from step 1.
3. Make sure TCP port 48898 is open in the Windows firewall on the PLC.

Your Python script works because it goes through the local TwinCAT router, which is why the TwinCAT router option above is the default.

## 6. Troubleshooting

| Error | Likely cause |
|---|---|
| 6 | PLC unreachable (wrong IP, firewall, no route) or wrong port |
| 7 | Target machine not found: route missing |
| 1808 | Symbol not found: check the name `MAIN.ncounter` and that the program is running |
| 1861 | Timeout: network or route problem |

## Notes

- Types must match the PLC: LREAL is `double`, INT is `int16_t`, DINT is `int32_t`, BOOL is `bool`. `main.cpp` wraps the INT at 32767 instead of overflowing.
- `AdsVariable` resolves the symbol handle once at start-up, so each cycle is a single ADS read or write.
- Next steps: add a device notification (event-driven reads) or write structs and arrays with `AdsVariable<std::array<...>>`.
