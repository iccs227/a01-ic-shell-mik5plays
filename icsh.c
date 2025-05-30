/* ICCS227: Project 1: icsh
 * Name: Theeradon Sarawek
 * StudentID: 6680210
 */

#include "stdio.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "sys/types.h"
#include "sys/wait.h"

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
// Redundant, as I will assume from now that unknown commands could be external
void command_unknown() {
    printf("Bad command.\n");
}

void command_external(const char *input) {

    // Say, half of the buffer could be arguments, at which we store them.
    char *args[MAX_CMD_BUFFER / 2 + 1];
    char buffer[MAX_CMD_BUFFER];
    strcpy(buffer, input);

    // Divide the arguments, null terminate at the end
    int i = 0;
    char *token = strtok(buffer, " ");
    while (token != NULL && i < MAX_CMD_BUFFER / 2) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (args[0] == NULL) {
        return; // No arguments, terminate
    }

    /*
     * "Your shell must spawn a new process,
     * execute it and wait for the command to
     * complete and resume control of the terminal."
     */
    pid_t pid = fork();
    if (pid == 0) {
        execvp(args[0], args);
        perror("Execution failed");
        exit(1);
    } else if (pid > 0) {
        int status;
        waitpid(pid, &status, 0);
    } else {
        perror("Fork failed");
    }
}

// Separate function for the shell, as we will use main() to handle arguments
int shell(FILE *input_stream, int script) {
    char buffer[MAX_CMD_BUFFER];
    char last_command[MAX_CMD_BUFFER] = "";

    printf("Starting IC shell...\n"); // Initialization

    while (1) {
        if (script == 0) { // Only print the prompt if we are not running a script
            printf("icsh $ ");
        }

        // Error handling
        if (fgets(buffer, MAX_CMD_BUFFER, input_stream) == NULL) {
            break;
        }

        // Handling command inputs, remove trailing newline
        buffer[strcspn(buffer, "\n")] = 0;

        // MILESTONE 1: !! (repeat last command)
        // If the last command is bad, it will repeat it anyway.
        if (strcmp(buffer, "!!") == 0) {
            if (last_command[0] == '\0') {
                if (script == 0) {
                    printf("No previous command!\n"); // Added notification before giving back prompt
                }
                continue;
            }
            strcpy(buffer, last_command); // Save last command as current command
        } else {
            strcpy(last_command, buffer); // Save current command as last command
        }

        if (strncmp(buffer, "echo ", 5) == 0) { // echo
            command_echo(buffer);
        } else if (strncmp(buffer, "exit", 4) == 0 && (buffer[4] == ' ' || buffer[4] == '\0')) {
            return command_exit(buffer);
        } else {
            command_external(buffer); // Assume it might be an external command
        }
    }

    return 0;
}
int main(int argc, char *argv[]) {
    // MILESTONE 2: Script Handling
    if (argc == 2) { // If there are arguments
        FILE *script = fopen(argv[1], "r");

        // It is imperative we do some error handling first
        if (!script) {
            perror("Can't open script, file may not exist/be invalid.");
            return 1;
        }

        int code = shell(script, 1); // We are running scripts
        fclose(script);
        return code;
    } else { // We are just running commands
        return shell(stdin, 0);
    }
}
