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

## Prerequisites

- TwinCAT 3 installed on a Windows PC, with a PLC project you can activate (or run in simulation)
- Python 3 with `pip` for the Python sample
- Visual Studio Build Tools (C++ workload), CMake and Git for the C++ sample
- The .NET 8 SDK for the C# sample

## Getting started

1. Create a TwinCAT 3 PLC project and put the variables from [`plc/MAIN.st`](plc/MAIN.st) in `MAIN`.
2. Activate it (or run in simulation) and set the PLC to Run.
3. Follow the README in the folder of the language you want.

## Sample output

```text
Connected to 10.0.0.111.1.1:851
Press Ctrl+C to stop.
Counter is = 161070
writecount to PLC= 1
Counter is = 161171
writecount to PLC= 2
```

The PLC counter advances between reads, so the exact numbers will differ on your system.

## Tested with

| Item | Version |
|---|---|
| TwinCAT 3 | Build 4026.27 (XAE and runtime, simulation on the same PC) |
| Windows | 11 |
| .NET SDK | 8.0.427 |
| CMake | 4.4 |
| C++ compiler | Visual Studio Build Tools 2022, MSVC 19.44 |

## Safety

These demos write values to PLC variables. Try them on a TwinCAT simulation or a test setup first, and never run them against a machine in operation unless you understand every variable they touch.

## Notes and lessons learned

- **Python** needs no compiler. **C++** needs a compiler that can build the ADS library and link TwinCAT's 64-bit `TcAdsDll.lib`; the old 32-bit MinGW 6.3 cannot. Visual Studio Build Tools (MSVC) worked. Check the licensing terms of any compiler against your own company's policy.
- With a **remote PLC** and no TwinCAT on the PC, the C++ sample can use the library's standalone router instead (`-DADS_USE_TWINCAT_ROUTER=OFF`), but then the PLC needs a static route back to the PC. That path is only compile-tested.
- ADS error **1808** means the symbol was not found (check the name and that the PLC program is running). Errors **6** and **7**, and timeouts, usually point to a wrong Net ID, a missing route, or a stopped PLC runtime.

## Disclaimer

Sample code, provided as is under the MIT license. TwinCAT and Beckhoff are trademarks of Beckhoff Automation GmbH & Co. KG. This repository is an independent project and is not affiliated with or endorsed by Beckhoff.

## License

MIT, see [`LICENSE`](LICENSE).
