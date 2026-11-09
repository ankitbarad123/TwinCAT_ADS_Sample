// =============================================================================
// AdsCounterDemo - C# equivalent of the Python and C++ counter demos
// =============================================================================
//
// What this program does, once per second:
//   * READS  MAIN.ncounter (PLC type LREAL -> C# type double)  from the TwinCAT 3 PLC
//   * WRITES MAIN.rcounter (PLC type INT   -> C# type short)   to   the TwinCAT 3 PLC
//
// How it talks to the PLC:
//   this program -> Beckhoff.TwinCAT.Ads (NuGet) -> TwinCAT router -> PLC runtime
//
// Usage (both arguments are optional):
//   dotnet run -- [AMS_NET_ID | local] [PORT]
//   dotnet run -- 10.0.0.111.1.1 851
//   dotnet run -- local 851            ("local" = the Net ID of the TwinCAT router on this PC)

using System;
using System.Threading;
using TwinCAT.Ads;  // AdsClient, AmsNetId (from the Beckhoff.TwinCAT.Ads NuGet package)

namespace AdsCounterDemo;

internal static class Program
{
    // Defaults taken from the Python script
    private const string DefaultNetId = "10.0.0.111.1.1";  // AMS Net ID of the PLC / TwinCAT router
    private const int DefaultPort = 851;                   // first PLC runtime (TwinCAT 3)

    // Symbol names exactly as declared in the PLC program
    private const string ReadSymbol = "MAIN.ncounter";   // LREAL
    private const string WriteSymbol = "MAIN.rcounter";  // INT

    private static int Main(string[] args)
    {
        // ---- Step 1: read the command-line arguments (or use the defaults) ----
        string netIdText = args.Length > 0 ? args[0] : DefaultNetId;
        int port = args.Length > 1 ? int.Parse(args[1]) : DefaultPort;

        // ---- Step 2: set up Ctrl+C handling ----
        // Pressing Ctrl+C cancels this token instead of killing the program, so the loop
        // ends normally and the connection is closed cleanly (like KeyboardInterrupt in Python).
        using var cts = new CancellationTokenSource();
        Console.CancelKeyPress += (_, e) =>
        {
            e.Cancel = true;  // do not terminate the process immediately
            cts.Cancel();
        };

        // Everything that can fail (connecting, reading, writing) is inside "try".
        try
        {
            // ---- Step 3: connect to the PLC (Python: pyads.Connection(...) + plc.open()) ----
            // "using" closes (disposes) the client automatically when the block ends,
            // which does the same job as plc.close() in Python's "finally".
            using var client = new AdsClient();

            AmsNetId netId = netIdText.Equals("local", StringComparison.OrdinalIgnoreCase)
                ? AmsNetId.Local          // Net ID of the TwinCAT router on this PC
                : new AmsNetId(netIdText);  // e.g. "10.0.0.111.1.1"

            client.Connect(netId, port);
            Console.WriteLine($"Connected to {netIdText}:{port}");
            Console.WriteLine("Press Ctrl+C to stop.");

            // ---- Step 4: the main loop (runs until Ctrl+C) ----
            short myVal = 0;  // value we write to MAIN.rcounter (counts 1, 2, 3, ...)
            while (!cts.IsCancellationRequested)
            {
                // READ from TC3 (Python: plc.read_by_name("MAIN.ncounter", PLCTYPE_LREAL))
                // ReadValue returns a generic "object", so we convert it to double.
                double readCount = (double)client.ReadValue(ReadSymbol, typeof(double));
                Console.WriteLine($"Counter is = {readCount}");

                // Increase our counter. An INT is 16-bit (max 32767), so go back to 0 at the
                // top instead of overflowing.
                myVal = myVal == short.MaxValue ? (short)0 : (short)(myVal + 1);

                // WRITE to TC3 (Python: plc.write_by_name("MAIN.rcounter", myval, PLCTYPE_INT))
                // The C# type of the value (short) decides how many bytes are sent: it must
                // match the PLC type (INT = 16-bit).
                client.WriteValue(WriteSymbol, myVal);
                Console.WriteLine($"writecount to PLC= {myVal}");

                // Wait one second, but wake up immediately if Ctrl+C is pressed
                cts.Token.WaitHandle.WaitOne(TimeSpan.FromSeconds(1));
            }

            Console.WriteLine();
            Console.WriteLine("Ctrl+C pressed");
        }
        catch (Exception ex)
        {
            // Connection problems and ADS errors end up here. The message includes the
            // ADS error code. Common ones: 1808 = symbol not found (check the name),
            // 6/7 = PLC or route not reachable, 1861 = timeout.
            Console.Error.WriteLine($"ADS Error: {ex.Message}");
            return 1;  // non-zero exit code = "something went wrong"
        }

        Console.WriteLine("PLC connection is closed");
        return 0;  // 0 = success
    }
}
