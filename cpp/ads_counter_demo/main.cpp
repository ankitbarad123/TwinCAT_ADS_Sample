// =============================================================================
// ads_counter_demo - C++ equivalent of adspy1.py
// =============================================================================
//
// What this program does, once per second:
//   * READS  MAIN.ncounter (PLC type LREAL -> C++ type double)  from the TwinCAT 3 PLC
//   * WRITES MAIN.rcounter (PLC type INT   -> C++ type int16_t) to   the TwinCAT 3 PLC
//
// How it talks to the PLC:
//   this program -> Beckhoff ADS library -> TcAdsDll.dll -> TwinCAT router -> PLC runtime
//
// Uses the open-source Beckhoff ADS library (https://github.com/Beckhoff/ADS).
//
// Usage (all arguments are optional; defaults are set below):
//   ads_counter_demo [PLC_AMS_NET_ID] [PLC_IP] [PLC_PORT] [LOCAL_AMS_NET_ID]
//   ads_counter_demo 10.0.0.111.1.1 10.0.0.111 851
//   ads_counter_demo 10.0.0.111.1.1 10.0.0.111 851 10.0.0.50.1.1

// -----------------------------------------------------------------------------
// Includes: code from other files that we want to use
// -----------------------------------------------------------------------------

// Beckhoff ADS library (downloaded by CMake)
#include "AdsLib.h"       // AdsDevice, AmsNetId, AdsException, route functions
#include "AdsVariable.h"  // AdsVariable<T>: read/write one PLC variable like a normal C++ variable

// C++ standard library
#include <atomic>     // std::atomic: a variable that is safe to change from a signal handler
#include <chrono>     // std::chrono::seconds: time durations for the sleep
#include <csignal>    // std::signal: detect Ctrl+C
#include <cstdint>    // int16_t, uint8_t, uint16_t: integers with an exact size in bits
#include <cstdlib>    // std::atoi: text -> number
#include <iostream>   // std::cout / std::cerr: print to the screen
#include <limits>     // std::numeric_limits: largest/smallest value of a type
#include <sstream>    // std::istringstream: read values out of a piece of text
#include <stdexcept>  // std::runtime_error: a standard error type
#include <string>     // std::string
#include <thread>     // std::this_thread::sleep_for: pause the program

// Everything inside this "anonymous namespace" is private to this file.
// It holds our helper constants and functions; main() is at the bottom.
namespace {

// -----------------------------------------------------------------------------
// Default settings (taken from the Python script)
// -----------------------------------------------------------------------------
// The IP is an ASSUMPTION: the AMS Net ID normally starts with the PLC's IP address,
// but check yours and pass it as argv[2].
// When built with the TwinCAT router (PLC on this PC / existing TwinCAT route), the IP and
// the LOCAL_AMS_NET_ID argument are ignored - the TwinCAT router does the routing.
//
// "constexpr" = a constant that is known when the program is compiled.
constexpr const char* kDefaultNetId = "10.0.0.111.1.1";  // AMS Net ID of the PLC / TwinCAT router
constexpr const char* kDefaultIp    = "10.0.0.111";      // IP of the PLC (unused with the TwinCAT router)
constexpr uint16_t    kDefaultPort  = 851;  // AMSPORT_R0_PLC_TC3, first PLC runtime

// -----------------------------------------------------------------------------
// parseNetId: turn the text "10.0.0.111.1.1" into an AmsNetId (6 bytes)
// -----------------------------------------------------------------------------
// Parses "a.b.c.d.e.f" into an AmsNetId. Done by hand because the TwinCAT-router build uses
// Beckhoff's plain AmsNetId struct (no string constructor), while the standalone build has one.
// Both variants expose the same public byte array `b[6]`.
AmsNetId parseNetId(const std::string& text)
{
    unsigned int part[6] = {};            // the six numbers of the Net ID, all start at 0
    char dot = 0;                         // used to read (and check) each '.' between numbers
    std::istringstream in(text);          // lets us read from the text like reading from the keyboard

    for (int i = 0; i < 6; ++i) {
        // Before every number except the first, there must be a '.'
        if (i > 0 && (!(in >> dot) || dot != '.')) {
            throw std::runtime_error("Invalid AMS Net ID '" + text + "' (expected a.b.c.d.e.f)");
        }
        // Read the number; it must exist and fit in one byte (0..255)
        if (!(in >> part[i]) || part[i] > 255) {
            throw std::runtime_error("Invalid AMS Net ID '" + text + "' (expected a.b.c.d.e.f)");
        }
    }

    // Copy the six numbers into the AmsNetId structure (one byte each)
    AmsNetId id{};
    for (int i = 0; i < 6; ++i) {
        id.b[i] = static_cast<uint8_t>(part[i]);
    }
    return id;
}

// -----------------------------------------------------------------------------
// Ctrl+C handling
// -----------------------------------------------------------------------------
// g_run is the "keep going" flag for the main loop. Pressing Ctrl+C sets it to false,
// so the loop ends normally and the program can clean up. (std::atomic makes it safe
// to change from the signal handler while the loop reads it.)
std::atomic<bool> g_run{true};

// Called by Windows when Ctrl+C is pressed (registered with std::signal in main)
void onSigint(int) { g_run = false; }

// -----------------------------------------------------------------------------
// adsErrorHint: friendly advice for the ADS error codes you are most likely to see
// -----------------------------------------------------------------------------
// Human-readable hints for the ADS errors you are most likely to hit.
const char* adsErrorHint(long code)
{
    switch (code) {
    case 6:    return "Could not reach the PLC (connection refused / no route) or the target port was not found.\n"
                      "      Check the IP, that TCP 48898 is open, that the PLC runtime is running, and the port (851).";
    case 7:    return "Target machine not found - the ADS route is missing on the PLC (or this PC).";
    case 18:   return "Port not connected / route not usable.";
    case 1792: return "Device error - check the PLC state.";
    case 1808: return "Symbol not found - check the variable name and that the PLC program is running and downloaded.";
    case 1809: return "Symbol version invalid - PLC was re-downloaded; reconnect.";
    case 1861: return "Timeout - check network, firewall (TCP 48898) and route.";
    default:   return "";  // unknown code: no hint (the error code itself is still printed)
    }
}

}  // namespace

// =============================================================================
// main: the program starts here
// =============================================================================
int main(int argc, char* argv[])
{
    // ---- Step 1: read the command-line arguments (or use the defaults) ----
    // argc = number of arguments (the program name itself counts as 1)
    // argv = the arguments as text: argv[1] = Net ID, argv[2] = IP, argv[3] = port, argv[4] = local Net ID
    const std::string netIdStr = (argc > 1) ? argv[1] : kDefaultNetId;
    const std::string plcIp    = (argc > 2) ? argv[2] : kDefaultIp;
    const uint16_t    plcPort  = (argc > 3) ? static_cast<uint16_t>(std::atoi(argv[3])) : kDefaultPort;

    // ---- Step 2: tell Windows to call onSigint() when the user presses Ctrl+C ----
    std::signal(SIGINT, onSigint);

    // Everything that can fail (connecting, reading, writing) is inside "try".
    // If an error happens, the matching "catch" below prints it (like Python's try/except).
    try {
        // ---- Step 3 (optional): set the AMS Net ID of THIS PC ----
        // Optional: force the AMS Net ID of THIS PC. Normally the library derives it from the
        // PC's IP address (e.g. 10.0.0.50 -> 10.0.0.50.1.1), and that is the Net ID you must
        // enter in the PLC's static route.
        if (argc > 4) {
            bhf::ads::SetLocalAddress(parseNetId(argv[4]));
        }

        // ---- Step 4: connect to the PLC (Python: pyads.Connection(...) + plc.open()) ----
        // Opens the route to the PLC. The route is removed again when `route` goes out of scope
        // (C++ cleans up automatically at the end of the block, so there is no plc.close() call).
        AdsDevice route{plcIp, parseNetId(netIdStr), plcPort};

        // ---- Step 5: get a handle to each PLC variable ----
        // Symbol handles are resolved once here (python's read_by_name does it on every call).
        // The type in <...> must match the PLC type: LREAL = double, INT = int16_t.
        AdsVariable<double>  ncounter{route, "MAIN.ncounter"};  // LREAL
        AdsVariable<int16_t> rcounter{route, "MAIN.rcounter"};  // INT

        std::cout << "Connected to " << netIdStr << ":" << plcPort << " (" << plcIp << ")\n"
                  << "Press Ctrl+C to stop.\n";

        // ---- Step 6: the main loop (runs until Ctrl+C sets g_run to false) ----
        int16_t myval = 0;  // the value we will write to MAIN.rcounter (counts 1, 2, 3, ...)
        while (g_run) {
            // READ from TC3: assigning the AdsVariable to a double reads it from the PLC
            // read var from TC3
            const double readcount = ncounter;
            std::cout << "Counter is = " << readcount << '\n';

            // Increase our counter. An INT is 16-bit (max 32767), so go back to 0 at the
            // top instead of overflowing (Python would raise an error here).
            // write var to TC3 (an INT is 16-bit, so wrap instead of overflowing)
            myval = (myval == std::numeric_limits<int16_t>::max()) ? int16_t{0}
                                                                   : static_cast<int16_t>(myval + 1);

            // WRITE to TC3: assigning a number to the AdsVariable writes it to the PLC
            rcounter = myval;
            std::cout << "writecount to PLC= " << myval << '\n';

            // Wait one second before the next cycle
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        std::cout << "\nCtrl+C pressed\n";

    // ---- Error handling ----
    } catch (const AdsException& ex) {
        // An ADS-specific error (wrong route, symbol not found, timeout, ...)
        std::cerr << "ADS Error " << ex.errorCode << ": " << ex.what() << '\n';
        const char* hint = adsErrorHint(ex.errorCode);
        if (*hint) {  // only print a hint if we have one for this code
            std::cerr << "Hint: " << hint << '\n';
        }
        return 1;  // non-zero exit code = "something went wrong"
    } catch (const std::exception& ex) {
        // Any other error, such as an invalid Net ID typed on the command line
        std::cerr << "Error: " << ex.what() << '\n';
        return 1;
    }

    // Reached after the loop ends normally. "route" has already been closed by now.
    std::cout << "PLC connection is closed\n";
    return 0;  // 0 = success
}
