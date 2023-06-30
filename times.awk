# Takes input with multiple run times (in POSIX time(1) format) and
# reduces them to a single line of the real, user and system times for
# fastest overall run.
#
# Example input:
#   real 0.37
#   user 0.06
#   sys 0.15
#   real 0.18
#   user 0.05
#   sys 0.12
#   real 0.19
#   user 0.05
#   sys 0.13
#
# Output:
#   0.18 0.05 0.12

BEGIN {
    REAL_T = ""
    USER_T = ""
    SYS_T = ""
}
/real/ {
    if (REAL_T == "" || REAL_T > $2) {
        REAL_T = $2;
        USER_T = "";
        SYS_T = "";
    }
}
/user/ {
    if (USER_T == "") USER_T = $2
}
/sys/ {
    if (SYS_T == "") SYS_T = $2
}
END {
    print REAL_T, USER_T, SYS_T
}
