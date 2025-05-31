/* ICCS227: Project 1: icsh
 * Name: Theeradon Sarawek
 * StudentID: 6680210
 */

#include "stdio.h"
#include "signal.h"
#include "string.h"
#include "stdlib.h"
#include "unistd.h"
#include "fcntl.h"
#include "sys/types.h"
#include "sys/wait.h"

#define MAX_CMD_BUFFER 255

// MILESTONE 4: Signal Handler
// We keep the foreground process's (SINGULAR) PID
// as well as storing the last exit status (default at 0)
volatile sig_atomic_t fg_pid = 0;
int last_exit_code = 0;

// MILESTONE 6: Background jobs + job control
// This area contains the "job" class/struct + some helper functions to
// help with job creation/termination.

// Node-like structure to store job information (linked list)
typedef struct job {
    int id;
    pid_t pid;
    char command[MAX_CMD_BUFFER];
    char status[16]; // running, stopped, done
    struct job *next;
} job;

job *job_list = NULL; // Only need 1 job list
int job_counter = 1; // Unique id for each job, start at ID=1 and increment after every job [different from PID]

// Add a new job, making sure it is appended at the end of the list too
void add_job(pid_t pid, const char *cmd) {
    job *new_job = malloc(sizeof(job));
    new_job->id = job_counter++;
    new_job->pid = pid;
    strncpy(new_job->command, cmd, MAX_CMD_BUFFER);
    strcpy(new_job->status, "Running");
    new_job->next = NULL;

    if (job_list == NULL) {
        job_list = new_job;
    } else {
        job *curr = job_list;
        while (curr->next != NULL) {
            curr = curr->next;
        }
        curr->next = new_job;
    }

    // [job id] process id to signify that we have created a job.
    printf("[%d] %d\n", new_job->id, pid);
}

// Remove a job
void remove_job(pid_t pid) {
    job *prev = NULL, *curr = job_list;
    while (curr != NULL) {
        if (curr->pid == pid) {
            if (prev == NULL) {
                job_list = curr->next;
            } else {
                prev->next = curr->next;
            }
            free(curr);
            return;
        }
        prev = curr;
        curr = curr->next;
    }
}







// MILESTONE 1: echo
// Edited for MILESTONE 4: Now prints the last exit code (default 0 for builtin commands)

// MILESTONE 5 NOTE:
// !! THIS IS NOW REDUNDANT !!
// Changed to make echo be handled as an external command, but
// have special case for $? that prints out the last exit code.
// but I will keep this here anyway as it is part of previous milestones.

void command_echo(const char *input) {
    const char *msg = input + 5;

    if (strcmp(msg, "$?") == 0) {
        printf("%d\n", last_exit_code);
    } else {
        printf("%s\n", msg);
    }

    last_exit_code = 0; // MILESTONE 4 edit

}

// MILESTONE 1: exit
int command_exit(const char *input) {
    int code = 0; // By default, let the exit code be 0 (if the user doesn't input anything)

    if (strncmp(input, "exit ", 5) == 0) {
        code = atoi(input + 5); // See if we have an exit code
        code = code & 0xFF; // Truncate to 8 bits (per instructions)
    }

    printf("Bye. Shell exited with code %d.\n", code);
    last_exit_code = 0; // MILESTONE 4 edit
    return code;
}

// Handling empty, unknown, invalid commands
// Redundant, as I will assume from now that unknown commands could be external
void command_unknown() {
    printf("Bad command.\n");
}

// MILESTONE 6: List all running and suspended jobs
// Just go through the linked list and print out each command that we queued
// Prints starting from the most recently called job.
void command_jobs() {
    job *curr = job_list;

    while (curr != NULL) {
        // [job id]   State   Command
        printf("[%d]  %s\t\t%s\n", curr->id, curr->status, curr->command);

        curr = curr->next;
    }
}

// MILESTONE 6: bring job to foreground
// Finds job, brings it to foreground, resumes if it is stopped, gets rid of it if its done
void command_fg(const char *input) {
    int job_id;
    // get job_id from user input
    if (sscanf(input, "fg %%%d", &job_id) != 1) {
        perror("Invalid Job ID. Try again");
        return;
    }

    // Find job
    job *curr = job_list;
    while (curr != NULL) {
        if (curr->id == job_id) { break; }
        curr = curr->next;
    }

    if (curr == NULL) {
        perror("Job not found");
        return;
    }

    // Print command that is being moved to foreground
    printf("%s\n", curr->command);

    // Send SIGCONT to resume the job if it was stopped
    if (kill(curr->pid, SIGCONT) < 0) {
        perror("SIGCONT failed");
        return;
    }

    fg_pid = curr->pid; // for signal handlers

    // Wait for job to finish or stop again
    int status;
    waitpid(curr->pid, &status, WUNTRACED);

    fg_pid = 0;

    // Update the exit code to reflect the foreground job, same code as in command_external
    if (WIFEXITED(status)) {
        last_exit_code = WEXITSTATUS(status);
    } else if (WIFSIGNALED(status)) {
        last_exit_code = 128 + WTERMSIG(status);
    } else {
        last_exit_code = 1;
    }

    // Get rid of job from the job list once its done
    if (WIFEXITED(status) || WIFSIGNALED(status)) {
        remove_job(curr->pid);
    }
}

// MILESTONE 6: Bring a suspended process to background
void command_bg(const char *input) {
    int job_id;
    // get job_id from user input
    // same stuff from command_fg
    if (sscanf(input, "bg %%%d", &job_id) != 1) {
        perror("Invalid Job ID. Try again");
        return;
    }

    // Find job
    job *curr = job_list;
    while (curr != NULL) {
        if (curr->id == job_id) { break; }
        curr = curr->next;
    }

    if (curr == NULL) {
        perror("Job not found");
        return;
    }

    if (kill(curr->pid, SIGCONT) < 0) {
        perror("Process cannot be continued");
        return;
    }
    // [job id]+     [command]
    strcpy(curr->status, "Running");
    printf("[%d]+ \t%s \n", curr->id, curr->command);
}

// MILESTONE 7:
// pusheen says: basically just echo except pusheen says your sentence
// represents the hardships that we went through during DSOOP
void command_psays(const char *input) {
    const char *msg = input + 12;

    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⣤⡀⠀⠀⠀⠀⠀⠀⠀⣀⣀⡀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣀⣀⣀⡀⠀⠀⠀⢀⣾⠋⠉⢿⣆⠀⠀⠀⠀⢀⣾⠟⠙⢿⣄⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣀⣤⣶⢿⣿⢻⡛⠛⢛⡟⠛⠛⠿⠿⠃⠀⠀⠀⠻⣿⢿⣿⢿⣿⠁⠀⠀⠀⢻⡆⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣴⡾⠋⠫⣀⡠⠃⠀⠑⠶⠚⣥⣀⣀⡀⠀⠀⠀⠀⠀⠀⠋⠀⠋⠈⠋⠀⠀⠀⠀⠈⣿⡀⣀⣀⣀⡀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⣠⡾⠋⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠉⣉⣉⣛⠃⠀⠀⣴⣦⠀⠀⠀⣦⠀⠀⢠⣶⡄⠀⠀⢸⣟⠛⠉⠉⠁\n");
    printf("⢰⣿⣿⣦⠀⠀⠀⠀⣰⡟⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠛⠛⠛⠛⠃⠀⠀⠛⠛⠀⠘⠿⠻⠟⠀⠘⠛⠃⠀⠀⠈⣿⠛⠛⠛⠃     < (%s)\n", msg);
    printf("⢸⣯⠟⠛⣷⡀⠀⢠⡿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡇⠀⠀⠀\n");
    printf("⠀⠹⣧⣴⣿⡿⢶⣾⠃⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⣇⠀⠀⠀\n");
    printf("⠀⠀⠘⢿⣮⡀⡞⢹⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠈⠛⢿⣾⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⢹⡇⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣿⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠈⣿⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢸⡇⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠹⣧⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢠⡿⠁⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠻⣦⡀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣰⡟⠁⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠙⢷⣄⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⢀⣤⡾⠋⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⣿⣤⡿⠛⠛⠛⠛⢿⣤⣿⠛⠛⠛⠛⠛⠛⠛⢿⣤⣾⠛⠛⠛⠛⠻⣧⣼⡏⠀⠀⠀⠀⠀⠀⠀⠀\n");
    printf("⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠀⠈⠉⠀⠀⠀⠀⠀⠀⠉⠁⠀⠀⠀⠀⠀⠀⠀⠀⠉⠁⠀⠀⠀⠀⠀⠉⠉⠀⠀⠀⠀⠀⠀⠀⠀⠀\n");

    last_exit_code = 0; // MILESTONE 4 edit
}

void command_external(const char *input) {

    // Say, half of the buffer could be arguments, at which we store them.
    char *args[MAX_CMD_BUFFER / 2 + 1];
    char buffer[MAX_CMD_BUFFER];
    strcpy(buffer, input);

    // MILESTONE 5: I/O redirection, so we need to have variables to store
    // both input and output file names (depending on direction of redirection)
    char *input_file = NULL;
    char *output_file = NULL;

    // Divide/tokenize the arguments, null terminate at the end
    // NEW for MILESTONE 5: Detecting redirection
    int i = 0;
    char *token = strtok(buffer, " ");
    while (token != NULL && i < MAX_CMD_BUFFER / 2) {
        if (strcmp(token, "<") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) break;
            input_file = token;
        } else if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            if (token == NULL) break;
            output_file = token;
        } else {
            args[i++] = token;
        }
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

    // MILESTONE 6 update for fork, detect if a background job is called

    int background = 0;

    // Check if last argument is "&" and remove it from both args and input
    if (i > 0 && strcmp(args[i - 1], "&") == 0) {
        background = 1;
        args[i - 1] = NULL;
    }

    pid_t pid = fork();

    if (pid == 0) { // If child,

        // Setting up both input and output file redirection.
        if (input_file) {
            // Flags: read only
            int fd_in = open(input_file, O_RDONLY);
            if (fd_in < 0) {
                perror("Input file error");
                exit(1);
            }
            dup2(fd_in, STDIN_FILENO); // syscall to redirect stdin from file and not terminal
            close(fd_in);
        }

        if (output_file) {
            // Flags: write, create if not exist, truncate if exist, 0644 (octal permissions rw-r--r--)
            int fd_out = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (fd_out < 0) {
                perror("Output file error");
                exit(1);
            }
            dup2(fd_out, STDOUT_FILENO); // syscall to redirect stdout to file
            close(fd_out);
        }
        execvp(args[0], args);
        perror("Execution failed");
        exit(1);
    } else if (pid > 0) { // If parent,
        // MILESTONE 4 edit for signal handling
        // MILESTONE 6 edit for background job creation

        if (background) {
            add_job(pid, input);
        } else {
            fg_pid = pid;
            int status;
            waitpid(pid, &status, WUNTRACED);
            fg_pid = 0;

            // See if child terminated correctly
            // Account for normal + signal exits.
            if (WIFEXITED(status)) {
                last_exit_code = WEXITSTATUS(status);
            } else if (WIFSIGNALED(status)) {
                last_exit_code = 128 + WTERMSIG(status);
            } else if (WIFSTOPPED(status)) {
                // MILESTONE 6 for CTRL+Z handling
                add_job(pid, input);
                // change status to Stopped instead of Running (which add_job does initially)
                job *curr = job_list;
                while (curr != NULL) {
                    if (curr->pid == pid) {
                        strcpy(curr->status, "Stopped");
                        break;
                    }
                    curr = curr->next;
                }
                printf("[%d]  Stopped\t%s\n", job_counter - 1, input);
                return;
            } else {
                last_exit_code = 1;
            }
        }

    } else {
        perror("Fork failed");
    }
}

// MILESTONE 4: Signal Handling
// Handler functions for both SIGINT and SIGTSTP
void handle_sigint(int signo) {
    if (fg_pid > 0) {
        kill(fg_pid, SIGINT);
    }
}

void handle_sigtstp(int signo) {
    if (fg_pid > 0) {
        kill(fg_pid, SIGTSTP);
    }
}


// MILESTONE 6: handle SIGCHLD (when child process terminates, or otherwise when job is done)
void handle_sigchld(int sig) {
    int status;
    pid_t pid;
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        job *curr = job_list;
        while (curr != NULL) {
            if (curr->pid == pid) {
                printf("\n[%d]+  Done\t\t%s\n", curr->id, curr->command);
                strcpy(curr->status, "Done");
                remove_job(pid);
                break;
            }
            curr = curr->next;
        }
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

        if (strcmp(buffer, "echo $?") == 0) {
            printf("%d\n", last_exit_code);
            last_exit_code = 0; // special echo to print out exit code
        } else if (strncmp(buffer, "echo ", 5) == 0) {
            command_external(buffer);  // treat all other echo as external, so supports redirection
        } else if (strncmp(buffer, "exit", 4) == 0 && (buffer[4] == ' ' || buffer[4] == '\0')) {
            return command_exit(buffer);
        } else if (strcmp(buffer, "jobs") == 0) {
            command_jobs();
            last_exit_code = 0;
        } else if (strncmp(buffer, "fg ", 3) == 0) {
            command_fg(buffer);
        } else if (strncmp(buffer, "bg ", 3) == 0) {
            command_bg(buffer);
        } else if (strncmp(buffer, "pusheensays ", 12) == 0) {
            command_psays(buffer);
        } else {
            command_external(buffer); // Assume it might be an external command
        }
    }

    return 0;
}
int main(int argc, char *argv[]) {
    // MILESTONE 2: Script Handling

    // MILESTONE 4: Installing signal handlers
    signal(SIGINT, handle_sigint);
    signal(SIGTSTP, handle_sigtstp);
    signal(SIGCHLD, handle_sigchld); // MILESTONE 6

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