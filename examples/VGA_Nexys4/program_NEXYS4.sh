#!/bin/bash
. /opt/Xilinx/Vivado/2019.1/settings64.sh >& /dev/null; 

vivado -mode batch -nojournal -nolog -source program_nexys4.tcl
