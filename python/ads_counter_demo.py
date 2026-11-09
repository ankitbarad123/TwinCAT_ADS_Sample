import pyads
import time

PLC_AMS_NET_ID = "10.0.0.111.1.1"
PLC_PORT = 851

plc = pyads.Connection(
    PLC_AMS_NET_ID,
    PLC_PORT
)

try:
    plc.open()
    myval = 0
    while True:
        # read var from tc3
        readcount = plc.read_by_name(
            "MAIN.ncounter",
            pyads.PLCTYPE_LREAL
        )
        print("Counter =", readcount)


        # write var to TC3
        myval = myval + 1
        writecount = plc.write_by_name(
           "MAIN.rcounter",
            myval,
           pyads.PLCTYPE_INT
       )

        print("writecount=", myval)
        time.sleep(1)
    


except KeyboardInterrupt:
    print("\nCtrl+C pressed")

except Exception as e:
    print("ADS Error:", e)

finally:
    plc.close()
    print("PLC connection closed")