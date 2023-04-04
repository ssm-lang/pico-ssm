# Tell orbtrace to provide power on vtref pin
!orbtrace -p vtref,3.3 -e vtref,on

# Start pyOCD GDB server in background
!killall pyocd
!pyocd gdb -t rp2040 -f10m &

# Configure GDB architecture
set arch arm

# Connect GDB with pyOCD
target extended-remote localhost:3333

# Enable semihosting
monitor arm semihosting enable

# Let areas beyond memory map be accessible
set mem inaccessible-by-default off

# Flash program
load
