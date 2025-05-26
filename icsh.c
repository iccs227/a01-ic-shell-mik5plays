/* ICCS227: Project 1: icsh
 * Name: Theeradon Sarawek
 * StudentID: 6680210
 */

#include "stdio.h"
#include "string.h"
#include "stdlib.h"

#define MAX_CMD_BUFFER 255

// MILESTONE 1: echo
void command_echo(const char *input) {
    printf("%s\n", input + 5); // Print everything after "echo "
}

// MILESTONE 1: exit
int command_exit(const char *input) {
    int code = 0; // By default, let the exit code be 0 (if the user doesn't input anything)

    if (strncmp(input, "exit ", 5) == 0) {
        code = atoi(input + 5); // See if we have an exit code
        code = code & 0xFF; // Truncate to 8 bits (per instructions)
    }

    printf("Bye. Shell exited with code %d.\n", code);
    return code;
}

// Handling empty, unknown, invalid commands
void command_unknown() {
    printf("Bad command.\n");
}

int main() {
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER] = "";

    printf("Starting IC shell...\n"); // Initialization

    while (1) {
        printf("icsh $ ");

        // Handling command inputs, remove trailing newline
        fgets(buffer, 255, stdin);
        buffer[strcspn(buffer, "\n")] = 0;

        // MILESTONE 1: !! (repeat last command)
        // If the last command is bad, it will repeat it anyway.
        if (strcmp(buffer, "!!") == 0) {
            if (last_command[0] == '\0') {
                printf("No previous command!\n"); // Added notification before giving back prompt
                continue;
            }
            strcpy(buffer, last_command); // Save last command as current command
        } else {
            strcpy(last_command, buffer); // Save current command as last command
        }

        if (strncmp(buffer, "echo ", 5) == 0) { // echo
            command_echo(buffer);
        } else if (strncmp(buffer, "exit", 4) == 0 && (buffer[4] == ' ' || buffer[4] == '\0')) {
            int code = command_exit(buffer);
            return code; // Exit with the specified code
        } else {
            command_unknown();
        }
    }

    return 0;
}
