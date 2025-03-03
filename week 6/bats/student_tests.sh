@test "Example: check ls runs without errors" {
    run ./dsh <<EOF
ls
EOF
    [ "$status" -eq 0 ]
}

@test "Exit command terminates shell" {
    run ./dsh <<EOF
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"cmd loop returned 0"* ]]
}

@test "cd with valid directory" {
    tmpdir=$(mktemp -d)
    run ./dsh <<EOF
cd $tmpdir
pwd
exit
EOF
    rmdir $tmpdir  # Cleanup
    [ "$status" -eq 0 ]
    [[ "$output" == *"$tmpdir"* ]]
}

@test "Error for non-existent command" {
    run ./dsh <<EOF
this_command_does_not_exist
EOF
    [ "$status" -eq 0 ]  
    [[ "$output" == *"The command does not found"* ]]
}

@test "Dragon command prints output" {
    run ./dsh <<EOF
dragon
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"Drexel Dragon"* ]] || [[ "$output" == *"ASCII Art"* ]]
}

@test "Empty command shows warning" {
    run ./dsh <<EOF
    
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"warning: no commands"* ]]
}

@test "cd to non-existent directory" {
    run ./dsh <<EOF
cd /nonexistent_directory
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"cd error"* ]]
}

@test "Long command input" {
    long_command=$(printf '%*s' 500 | tr ' ' 'a')
    run ./dsh <<EOF
$long_command
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"The command does not found"* ]]
}

@test "Verify rc command returns last command status" {
    run ./dsh <<EOF
rc
false
rc
exit
EOF
    return_codes=($(echo "$output" | sed -E 's/dsh2> //g' | grep -E '^[0-9]+$' | tail -n 2))
    [ "${#return_codes[@]}" -ge 2 ]
    [ "${return_codes[0]}" -eq 0 ]
    [ "${return_codes[1]}" -eq 1 ]
}
