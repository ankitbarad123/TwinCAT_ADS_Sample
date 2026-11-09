# ADS counter demo (C#)

C# equivalent of the Python and C++ counter demos. Once per second it reads `MAIN.ncounter` (LREAL) and writes an incrementing value to `MAIN.rcounter` (INT), using Beckhoff's official [`Beckhoff.TwinCAT.Ads`](https://www.nuget.org/packages/Beckhoff.TwinCAT.Ads) NuGet package.

**Status:** tested on Windows against a TwinCAT 3 simulation on the same PC (Net ID `10.0.0.111.1.1`, port 851).

## 1. PLC side

Use the variables in [`../../plc/MAIN.st`](../../plc/MAIN.st) (`ncounter : LREAL`, `rcounter : INT`) and set the PLC to Run (a TwinCAT simulation on the same PC is fine).

## 2. Install the .NET SDK

Install the .NET 8 (or newer) SDK from <https://dotnet.microsoft.com/download>, then check it in a new PowerShell window:

```powershell
dotnet --version
```

TwinCAT must be installed on the PC (XAE or XAR) so that the TwinCAT ADS router is available. The package supports running without a full TwinCAT install by other means (see the package page), but that is not covered here.

## 3. Build and run

```powershell
cd csharp/AdsCounterDemo
dotnet run -- 10.0.0.111.1.1 851
```

The first run downloads the NuGet package, so it needs internet access. Use `local` instead of a Net ID to connect to the local TwinCAT router: `dotnet run -- local 851`. Press Ctrl+C to stop.

Expected output (values will differ):

```text
Connected to 10.0.0.111.1.1:851
Press Ctrl+C to stop.
Counter is = 161070
writecount to PLC= 1
```

## 4. How it maps to the Python and C++ versions

| Step | Python (pyads) | C# | C++ |
|---|---|---|---|
| Connect | `pyads.Connection(netId, 851)`, `open()` | `new AdsClient()`, `Connect(netId, 851)` | `AdsDevice route{ip, netId, 851}` |
| Read LREAL | `read_by_name(..., PLCTYPE_LREAL)` | `(double)client.ReadValue("MAIN.ncounter", typeof(double))` | `AdsVariable<double>` |
| Write INT | `write_by_name(..., PLCTYPE_INT)` | `client.WriteValue("MAIN.rcounter", (short)v)` | `AdsVariable<int16_t>` |
| Close | `plc.close()` | `using var client` (automatic) | automatic when the object ends |

The C# type of the value decides how many bytes are sent, so it must match the PLC type: INT is `short`, DINT is `int`, REAL is `float`, LREAL is `double`, BOOL is `bool`.

## 5. Troubleshooting

| Symptom | Likely cause |
|---|---|
| `dotnet` not recognized | The SDK is not installed, or open a new PowerShell window after installing |
| Restore or build error about the package | No internet access, or the package version is not available. Pin a version in `AdsCounterDemo.csproj` |
| Error about `AmsNetId` or `Connect` not compiling | Package API differs from this sample. Paste the error and the package version (`dotnet list package`) |
| ADS error 1808 | Symbol not found: check the name `MAIN.ncounter` and that the PLC program is running |
| ADS error 6 or 7, or a timeout | PLC or route not reachable: check the Net ID, the port (851), and that TwinCAT is running |
