#!/bin/bash

milk << EOF
mload uptheramp
readshmim a
#readshmim b
#readshmim c
utr.cred_ql_utr ..procinfo 1
utr.cred_ql_utr ..triggermode 1
#utr.cred_ql_utr ..triggersname "a"
#utr.cred_ql_utr ..triggerdelay 0.001
utr.cred_ql_utr ..loopcntMax -1
utr.cred_ql_utr a b c 5000
listim
exitCLI
EOF
