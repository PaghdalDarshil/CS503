#!/usr/bin/env bats

# File: student_tests.sh
# 
# Create your unit tests suit in this file

@test "Example: check ls runs without errors" {
    run ./dsh <<EOF                
ls
EOF

    # Assertions
    [ "$status" -eq 0 ]
}

@test "Exit command terminates shell" {
    run ./dsh <<EOF
exit
EOF
    # Should see normal exit (0) and loop termination message
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
    # Cleanup test directory
    rmdir $tmpdir
    [ "$status" -eq 0 ]
    [[ "$output" == *"$tmpdir"* ]]
}

@test "Error for non-existent command" {
    run ./dsh <<EOF
this_command_does_not_exist
EOF
    [ "$status" -eq 0 ]  # Shell should continue running
    [[ "$output" == *"The command does not found"* ]]
}

@test "Dragon command prints output" {
    run ./dsh <<EOF
dragon
exit
EOF
    [ "$status" -eq 0 ]
    # Match either the header or actual dragon output
    [[ "$output" == *"Drexel Dragon"* ]] || [[ "$output" == *"ASCII Art"* ]]
}

@test "Empty command shows warning" {
    run ./dsh <<EOF
      
    # Just spaces and enter
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"warning: no commands"* ]]
}