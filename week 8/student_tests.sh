#!/usr/bin/env bats

# File: student_tests.sh
# 
# Custom unit tests for dsh

DSH_EXEC="./dsh"

# Server settings
SERVER_IP="127.0.0.1"
SERVER_PORT="5678"

# Start the server in the background before tests
setup() {
    # Kill any existing dsh server instances
    pkill -f "$DSH_EXEC -s" || true

    # Start the server in the background
    $DSH_EXEC -s -i 0.0.0.0 -p $SERVER_PORT > server_output.log 2>&1 &
    sleep 1  # Allow server time to start
}

# Stop the server after tests
teardown() {
    pkill -f "$DSH_EXEC -s" || true
    rm -f server_output.log client1_output.log client2_output.log
}

@test "Basic command - ls" {
    run $DSH_EXEC <<EOF
ls
exit
EOF
    echo "$output"
    [[ "$output" =~ "dsh4>" ]]
}

@test "Basic command - pwd" {
    run $DSH_EXEC <<EOF
pwd
exit
EOF
    echo "$output"
    [[ "$output" =~ "/" ]]
}

@test "Command with arguments - echo" {
    run $DSH_EXEC <<EOF
echo "Hello dsh!"
exit
EOF
    echo "$output"
    [[ "$output" =~ "Hello dsh!" ]]
}

@test "Pipeline - ls | grep dshlib.c" {
    run $DSH_EXEC <<EOF
ls | grep dshlib.c
exit
EOF
    echo "$output"
    [[ "$output" =~ "dshlib.c" ]]
}

@test "Output redirection - echo 'test' > testfile" {
    run $DSH_EXEC <<EOF
echo 'test' > testfile
exit
EOF
    run cat testfile
    echo "$output"
    [[ "$output" =~ "test" ]]
    rm -f testfile
}

@test "Built-in command - cd" {
    run $DSH_EXEC <<EOF
cd ..
pwd
exit
EOF
    echo "$output"
    [[ "$output" =~ "/" ]]
}

@test "Invalid command" {
    run $DSH_EXEC <<EOF
invalidcmd
exit
EOF
    echo "$output"
    [[ "$output" =~ "execvp failed" ]]
}

@test "Multi-threaded support test" {
    # Start multi-threaded server
    pkill -f "$DSH_EXEC -s" || true
    $DSH_EXEC -s -i 0.0.0.0 -p $SERVER_PORT -x > server_output.log 2>&1 &
    sleep 1

    # Run two clients simultaneously
    $DSH_EXEC -c -i $SERVER_IP -p $SERVER_PORT <<EOF > client1_output.log 2>&1 &
ls
exit
EOF

    $DSH_EXEC -c -i $SERVER_IP -p $SERVER_PORT <<EOF > client2_output.log 2>&1 &
whoami
exit
EOF

    sleep 2  # Give clients time to execute

    # Validate both clients received expected output
    run cat client1_output.log
    echo "$output"
    [[ "$output" =~ "dsh4>" ]]

    run cat client2_output.log
    echo "$output"
    [[ "$output" =~ "dsh4>" ]]

    rm -f client1_output.log client2_output.log
}
