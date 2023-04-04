# Start pyOCD GDB server in background
!killall pyocd
!pyocd gdb -t rp2040 &

# Configure GDB architecture
set arch arm

# Connect GDB with pyOCD
target extended-remote localhost:3333

# Let areas beyond memory map be accessible
set mem inaccessible-by-default off

# Flash program
load
