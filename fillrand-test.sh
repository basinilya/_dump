#!/bin/sh
${CC:?} -o fillrand-test fillrand-test.c || exit 1

x() {
    echo "$@"
    time ./fillrand-test 50 "$@"
    echo
    echo
}

x dumbfillrand

x wfillrand32_31_disp
x fillrand32_31_disp   

x wfillrand64_31_disp
x fillrand64_31_disp   



x fillrand32_31_nodisp 
x fillrand32_15_disp   
x fillrand32_15_nodisp 
x fillrand64_31_nodisp 
x fillrand64_15_disp   
x fillrand64_15_nodisp 

x wfillrand32_31_nodisp
x wfillrand32_15_disp
x wfillrand32_15_nodisp
x wfillrand64_31_nodisp
x wfillrand64_15_disp
x wfillrand64_15_nodisp

exit 0
