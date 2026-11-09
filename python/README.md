# ADS counter demo (Python)

The original counter demo, using [pyads](https://pypi.org/project/pyads/). It reads `MAIN.ncounter` (LREAL) and writes an incrementing value to `MAIN.rcounter` (INT) once per second.

**Status:** tested against a TwinCAT 3 simulation on the same PC (Net ID `10.0.0.111.1.1`, port 851).

## Run

```powershell
pip install -r requirements.txt
python ads_counter_demo.py
```

Edit `PLC_AMS_NET_ID` and `PLC_PORT` at the top of the script for your system. The PLC needs the variables from [`../plc/MAIN.st`](../plc/MAIN.st) and must be in Run mode. Press Ctrl+C to stop.
