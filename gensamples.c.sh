#!/bin/bash
set -e
{
cd samples

LC_ALL=C


cat <<'EOF'
#include "saytimespan.h"

struct samples_entry saytimespan_samples[] = {
EOF

comma=
for f in *.wav; do
    echo "$comma { \"${f%.wav}\" }"
    comma=','
done

cat <<'EOF'
};

size_t saytimespan_samples_count = sizeof(saytimespan_samples)/sizeof(saytimespan_samples[0]);

EOF
} >samples.c
