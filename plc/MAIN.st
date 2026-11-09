(*
  Minimal PLC side for the counter demos (Python, C++ and C#).

  Put these variables in the MAIN program of a TwinCAT 3 PLC project, build it,
  activate the configuration (or run it in simulation) and set the PLC to Run.

  The demos then:
    - READ  ncounter  (LREAL) from the PLC
    - WRITE rcounter  (INT)   to the PLC
*)
PROGRAM MAIN
VAR
    ncounter : LREAL;   // counts up on every PLC cycle; read by the demo programs
    rcounter : INT;     // written by the demo programs
END_VAR

ncounter := ncounter + 1;
