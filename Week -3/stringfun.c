#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BUFFER_SZ 50

// Prototypes
void usage(char *);
void print_buff(char *, int);
int setup_buff(char *, char *, int);
int count_words(char *, int, int);
//add additional prototypes here
void reverse_string(char *, int);
void print_words(char *, int, int);

int setup_buff(char *buff, char *user_str, int len) {
    // TODO: #4: Implement the setup_buff as per the directions
    char *src = user_str;
    char *dest = buff;
    int char_count = 0;
    int in_whitespace = 1;

    while (*src != '\0') {
        if (char_count >= len) return -1; // Input string exceeds buffer size

        char c = (*src == '\t') ? ' ' : *src; // Replace tabs with spaces
        if (c == ' ') {
            if (!in_whitespace) {
                *dest++ = ' ';
                char_count++;
                in_whitespace = 1;
            }
        } else {
            *dest++ = c;
            char_count++;
            in_whitespace = 0;
        }
        src++;
    }

    if (char_count > 0 && *(dest - 1) == ' ') {
        dest--;
        char_count--;
    }

    memset(dest, ' ', len - char_count); // Fill remaining buffer with '.'
    return char_count;
}

void print_buff(char *buff, int len) {
    printf("Buffer:  ");
    for (int i = 0; i < len; i++) {
        putchar(*(buff + i));
    }
    putchar('\n');
}

void usage(char *exename) {
    printf("usage: %s [-h|c|r|w] \"string\" [other args]\n", exename);
}

int count_words(char *buff, int len, int str_len) {
    // TODO: Implement the count_words functionality
    int count = 0;
    int in_word = 0;

    for (int i = 0; i < str_len; i++) {
        if (buff[i] == ' ') {
            in_word = 0;
        } else if (!in_word) {
            count++;
            in_word = 1;
        }
    }

    return count;
}

void reverse_string(char *buff, int str_len) {
    char *start = buff;
    char *end = buff + str_len - 1;

    while (start < end) {
        char temp = *start;
        *start = *end;
        *end = temp;
        start++;
        end--;
    }
}
//ADD OTHER HELPER FUNCTIONS HERE FOR OTHER REQUIRED PROGRAM OPTIONS

void print_words(char *buff, int len, int str_len) {
    int word_count = 0;
    char *current = buff;
    char *word_start = NULL;
    int in_word = 0;

    printf("Word Print\n----------\n");
    for (int i = 0; i < str_len; i++) {
        if (*current == ' ') {
            if (in_word) {
                printf("%d. ", ++word_count);
                for (char *p = word_start; p < current; p++) putchar(*p);
                printf(" (%ld)\n", current - word_start);
                in_word = 0;
            }
        } else if (!in_word) {
            word_start = current;
            in_word = 1;
        }
        current++;
    }

    if (in_word) {
        printf("%d. ", ++word_count);
        for (char *p = word_start; p < current; p++) putchar(*p);
        printf(" (%ld)\n", current - word_start);
    }
}

int main(int argc, char *argv[]) {
    char *buff;             // Placeholder for the internal buffer
    char *input_string;     // Holds the string provided by the user on cmd line
    char opt;               // Used to capture user option from cmd line
    int rc;                 // Used for return codes
    int user_str_len;       // Length of user supplied string

    // TODO: #1 Why is this safe, aka what if argv[1] does not exist?
    /* Ans: This is safe because the condition (argc < 2) ensures that argv[1] exists. 
    And if it doesn't, then the program exits before accessing argv[1].*/ 
    if ((argc < 2) || (*argv[1] != '-')) {
        usage(argv[0]);
        exit(1);
    }

    opt = (char)*(argv[1] + 1);   // Get the option flag

    // Handle the help flag and then exit normally
    if (opt == 'h') {
        usage(argv[0]);
        exit(0);
    }
    //WE NOW WILL HANDLE THE REQUIRED OPERATIONS
    // TODO: #2 Document the purpose of the if statement below
    /* Ans: This checks if the user has provided the required input string.
    And if not,then it exits the program with an error message.*/
    if (argc < 3) {
        usage(argv[0]);
        exit(1);
    }

    input_string = argv[2]; // Capture the user input string

    // TODO: #3 Allocate space for the buffer using malloc and
    // handle error if malloc fails by exiting with a return code of 99.
    buff = (char *)malloc(BUFFER_SZ);
    if (!buff) {
        printf("Error: Memory allocation failed\n");
        exit(99);
    }

    user_str_len = setup_buff(buff, input_string, BUFFER_SZ);
    if (user_str_len < 0) {
        printf("Error setting up buffer, error = %d\n", user_str_len);
        free(buff);
        exit(2);
    }

    switch (opt) {
        case 'c': // Count words
            rc = count_words(buff, BUFFER_SZ, user_str_len);
            printf("Word Count: %d\n", rc);
            break;

        case 'r': // Reverse string
            reverse_string(buff, user_str_len);
            printf("Reversed String: ");
            for (int i = 0; i < user_str_len; i++) putchar(buff[i]);
            printf("\n");
            break;

        case 'w': // Print words
            print_words(buff, BUFFER_SZ, user_str_len);
            break;

        default:
            usage(argv[0]);
            free(buff);
            exit(1);
    }

    // TODO: #6 Don't forget to free your buffer before exiting
    // Ans: Freeing the buffer prevents memory leaks.
    print_buff(buff, BUFFER_SZ);
    free(buff);
    exit(0);
}

// TODO: #7 Why do you think providing both the pointer and the length is a good practice?
/* Ans: Providing both the pointer and the length ensures the function does not assume a fixed buffer size, avoids overflows, 
and makes the function reusable for different buffer sizes.And it also clarifies how much of the buffer is valid data versus padding
or empty space.*/
