@test "Basic command execution: ls" {
    run ./dsh <<EOF
ls
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Pipeline: ls | grep dshlib.c" {
    run ./dsh <<EOF
ls | grep dshlib.c
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"dshlib.c"* ]]
}

@test "Pipeline: ls | grep .c | wc -l" {
    run ./dsh <<EOF
ls | grep .c | wc -l
exit
EOF
    [ "$status" -eq 0 ]
}

@test "Shell Exit Verification" {
    run ./dsh <<EOF
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "exit" ]]
}

@test "Empty input should not cause errors" {
    run ./dsh <<EOF

exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "warning: no commands provided" ]]
}

@test "Invalid command should print error" {
    run ./dsh <<EOF
invalidcommand123
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "execvp failed" ]]
}

@test "Built-in command: cd and pwd verification" {
    run ./dsh <<EOF
cd /tmp
pwd
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" =~ "/tmp" ]]
}

@test "Output Redirection: echo Hello > test_output.txt" {
    rm -f test_output.txt 
    run ./dsh <<EOF
echo Hello > test_output.txt
exit
EOF
    sleep 1  
    [ "$status" -eq 0 ]
    if [ ! -f test_output.txt ]; then
        echo "Test Failed: File test_output.txt was not created!"
        exit 1
    fi
    content=$(cat test_output.txt | tr -d '\r')
    [[ "$content" == *"Hello"* ]]
    rm -f test_output.txt
}

@test "Input Redirection: wc -w < test_input.txt" {
    echo "Hello World" > test_input.txt
    run ./dsh <<EOF
wc -w < test_input.txt
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$output" == *"2"* ]]
    rm -f test_input.txt
}

@test "Append to file: echo 'New Line' >> test_append.txt" {
    rm -f test_append.txt
    echo "First Line" > test_append.txt
    run ./dsh <<EOF
echo "Second Line" >> test_append.txt
exit
EOF
    [ "$status" -eq 0 ]
    [[ "$(cat test_append.txt)" == *"First Line"* ]]
    [[ "$(cat test_append.txt)" == *"Second Line"* ]]
    rm -f test_append.txt
}
