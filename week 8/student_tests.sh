DSH_EXEC="./dsh"

SERVER_IP="127.0.0.1"
SERVER_PORT="5678"

setup() {
    pkill -f "$DSH_EXEC -s" || true
    $DSH_EXEC -s -i 0.0.0.0 -p $SERVER_PORT > server_output.log 2>&1 &
    sleep 1 
}

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

@test "Invalid command" {
    run $DSH_EXEC <<EOF
invalidcmd
exit
EOF
    echo "$output"
    [[ "$output" =~ "execvp failed" ]]
}

@test "Multi-threaded support test" {
    pkill -f "$DSH_EXEC -s" || true
    $DSH_EXEC -s -i 0.0.0.0 -p $SERVER_PORT -x > server_output.log 2>&1 &
    sleep 1
    $DSH_EXEC -c -i $SERVER_IP -p $SERVER_PORT <<EOF > client1_output.log 2>&1 &
ls
exit
EOF

    $DSH_EXEC -c -i $SERVER_IP -p $SERVER_PORT <<EOF > client2_output.log 2>&1 &
whoami
exit
EOF

    sleep 2
    run cat client1_output.log
    echo "$output"
    [[ "$output" =~ "dsh4>" ]]

    run cat client2_output.log
    echo "$output"
    [[ "$output" =~ "dsh4>" ]]

    rm -f client1_output.log client2_output.log
}
