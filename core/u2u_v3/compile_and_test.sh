#!/bin/bash

set -e

cleaning() {
    make clean
    if [ -f "test_messages_for_u2u_core.txt" ]; then
        rm test_messages_for_u2u_core.txt
        touch test_messages_for_u2u_core.txt
    fi
    
    if [ -f "logs/main_tester_log.txt" ]; then
        rm logs/*.txt
    fi
    
    if [ -f "testing/u2u_core_tester.log" ]; then
        rm testing/u2u_core_tester.log
    fi
}

# Dependencies that need to be compiled together based on UPC:
obj_arr=(
"OBJC = Bogus line to align indexing. If this is read in Makefile something went terribly wrong with $0"
"OBJC = obj/u2u.o obj/u2u_HAL_lx.o obj/c_logger.o"
"OBJC = obj/u2u.o obj/u2u_HAL_pico.o obj/c_logger.o obj/hardware/uart.o obj/hardware/gpio.o"
"OBJC = obj/u2u.o obj/u2u_HAL_esp.o obj/c_logger.o"
)

compile() {
    echo "Testing with U2U_PLATFORM_CHANNELS Set to $1:"
    # Replacing UPC parameter:
    sed -i "s/#define U2U_PLATFORM_CHANNELS.*/#define U2U_PLATFORM_CHANNELS $1/" src/u2uclientdef.h
    new_line="${obj_arr[$1]}"
    # Replacing dependencies in Makefile:
    sed -i "$(printf '/OBJC =/c\\\n%s\n' "$new_line")" Makefile
    grep -in -C 2  "OBJC =" Makefile
    make -j4
}
testing() {
    echo " #python3 testing/u2u_core_tester.py --upc $1"
    python3 testing/u2u_core_tester.py --upc $1
}

default_action(){
    for i in {1..3}; do
        cleaning
        compile $i
        testing $i
    done
}

if [[ $# -eq 0 ]]; then 
    default_action
else
  if [[ $1 == "--upc" ]]; then
    # Check if the option value is provided and is an integer
    if [[ $# -eq 2 && $2 =~ ^[0-9]+$ && $2 -ge 1 && $2 -le 3 ]]; then
        cleaning
        compile $2
        testing $2
    else
      echo "Invalid option value. Please use --upc <1|2|3>"
      exit 1
    fi
  else
    echo "Invalid option. Please use --upc <1|2|3> or no options"
    exit 1
  fi
fi
   
