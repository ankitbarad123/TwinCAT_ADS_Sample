# TwinCAT 3 ADS samples

The same small ADS demo in three languages: read a variable from a TwinCAT 3 PLC and write another one, once per second. Use it as a starting point for talking to a TwinCAT PLC from your own Python, C++ or C# programs.

| Folder | Language | Library | Status |
|---|---|---|---|
| [`python/`](python) | Python | [pyads](https://pypi.org/project/pyads/) | Tested against a TwinCAT 3 simulation |
| [`cpp/ads_counter_demo/`](cpp/ads_counter_demo) | C++ (CMake, MSVC) | [Beckhoff ADS library](https://github.com/Beckhoff/ADS) with the local TwinCAT router (TcAdsDll) | Tested against a TwinCAT 3 simulation on Windows |
| [`csharp/AdsCounterDemo/`](csharp/AdsCounterDemo) | C# (.NET 8) | [Beckhoff.TwinCAT.Ads](https://www.nuget.org/packages/Beckhoff.TwinCAT.Ads) (NuGet) | Tested against a TwinCAT 3 simulation on Windows |
| [`plc/`](plc) | Structured Text | | The two variables the demos use |

## What the demos do

Each program connects to the PLC runtime (AMS port 851), then every second:

1. reads `MAIN.ncounter` (`LREAL`, a counter the PLC increments), and
2. writes an incrementing number to `MAIN.rcounter` (`INT`).

## How it works

```text
your program -> ADS library -> TwinCAT ADS router (AMS Net ID) -> PLC runtime (port 851) -> variables
```

- An ADS address is an **AMS Net ID** (which machine) plus a **port** (which program on it). Port 851 is the first TwinCAT 3 PLC runtime.
- The data types must match on both sides. ADS moves raw bytes, so LREAL is 8 bytes (`double`) and INT is 2 bytes (`int16_t` / `short`).
- All three samples use the TwinCAT router already running on the PC, so a PLC on the same PC (for example a simulation) needs no extra route.

## Getting started

1. Create a TwinCAT 3 PLC project and put the variables from [`plc/MAIN.st`](plc/MAIN.st) in `MAIN`.
2. Activate it (or run in simulation) and set the PLC to Run.
3. Follow the README in the folder of the language you want.

## Notes and lessons learned

- **Python** needs no compiler. **C++** needs a compiler that can build the ADS library and link TwinCAT's 64-bit `TcAdsDll.lib`; the old 32-bit MinGW 6.3 cannot. Visual Studio Build Tools (MSVC) worked. Check the licensing terms of any compiler against your own company's policy.
- With a **remote PLC** and no TwinCAT on the PC, the C++ sample can use the library's standalone router instead (`-DADS_USE_TWINCAT_ROUTER=OFF`), but then the PLC needs a static route back to the PC. That path is only compile-tested.
- ADS error **1808** means the symbol was not found (check the name and that the PLC program is running). Errors **6** and **7**, and timeouts, usually point to a wrong Net ID, a missing route, or a stopped PLC runtime.


