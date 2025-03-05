@test "Basic command execution: ls" {
    run ./dsh <<EOF
ls
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Pipes: Simple command (ls | grep dshlib.c)" {
    run ./dsh <<EOF
ls | grep dshlib.c
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"dshlib.c"* ]]
}

@test "Pipes: Multiple commands (ls | grep .c | wc -l)" {
    run ./dsh <<EOF
ls | grep .c | wc -l
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Exit command terminates shell" {
    run ./dsh <<EOF
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"exiting..."* ]]
}
