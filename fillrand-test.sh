#!/bin/sh
(
gcc -O2 -o fillrand-test fillrand-test.c
x() {
    echo "$@"
    time ./fillrand-test 50 "$@"
    echo
    echo
}
x fillrand32_31_disp   
x fillrand32_31_nodisp 
x fillrand32_15_disp   
x fillrand32_15_nodisp 
x fillrand64_31_disp   
x fillrand64_31_nodisp 
x fillrand64_15_disp   
x fillrand64_15_nodisp 
)