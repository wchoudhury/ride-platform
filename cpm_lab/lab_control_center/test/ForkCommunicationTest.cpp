#include <chrono>
#include <cstring>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

//To spawn a process & get its PID
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>

enum PROCESS_STATE {
    RUNNING, ERROR, DONE
};

int execute_command_get_pid(const char* cmd)
{
    int process_id = fork();
    if (process_id == 0)
    {
        //Tell the child to set its group process ID to its process ID, or else things like kill(-pid) to kill a ping-while-loop won't work
        setpgid(0, 0);
        
        //Actions to take within the new child process
        //ACHTUNG: NUTZE chmod u+x für die Files, sonst: permission denied
        execl("/bin/sh", "bash", "-c", cmd, NULL);

        //Error if execlp returns
        std::cerr << "Execl error in Deploy class: %s, for execution of '%s'" << std::strerror(errno) << cmd << std::endl;

        exit(EXIT_FAILURE);
    }
    else if (process_id > 0)
    {
        //We are in the parent process and got the child's PID
        return process_id;
    }
    else 
    {
        //We could not spawn a new process - usually, the program should not just break at this point, unless that behaviour is desired
        //TODO: Change behaviour
        std::cerr << "Error in Deploy class: Could not create child process for " << cmd << std::endl;
        exit(EXIT_FAILURE);
    }
}

PROCESS_STATE get_child_process_state(int process_id)
{
    int process_status;
    pid_t result = waitpid(process_id, &process_status, WNOHANG);

    if (result == 0)
    {
        return PROCESS_STATE::RUNNING;
    }
    else if (result == -1)
    {
        return PROCESS_STATE::ERROR;
    }
    else
    {
        return PROCESS_STATE::DONE;
    }
    
}

void kill_process(int process_id)
{
    //Tell the process to terminate - this way, it can terminate gracefully
    //We mostly use bash, were whole process groups might be created - to kill those, we need the negative id
    if (process_id > 0)
    {
        process_id *= (-1);
    }

    kill(process_id, SIGTERM);

    //Wait for the process to terminate
    std::this_thread::sleep_for(std::chrono::seconds(3));

    //Check if the process terminated as desired
    PROCESS_STATE state = get_child_process_state(process_id);

    //If the process has not yet terminated, force a termination and clean up
    if (state != PROCESS_STATE::DONE)
    {
        kill(process_id, SIGKILL);
        int status;
        waitpid(process_id, &status, 0); //0 -> no flags here
    }
}

bool spawn_and_manage_process(const char* cmd, unsigned int timeout_seconds)
{
    std::cout << "Executing " << cmd << std::endl;

    //Spawn and manage new process
    int process_id = execute_command_get_pid(cmd);
    auto start_time = std::chrono::high_resolution_clock::now();

    auto time_passed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
    //Regularly check status during execution until timeout - exit early if everything worked as planned, else run until error / timeout and return error
    while (time_passed_ms < static_cast<int64_t>(timeout_seconds) * 1000)
    {
        //Check current program state
        PROCESS_STATE state = get_child_process_state(process_id);

        if (state == PROCESS_STATE::DONE)
        {
            std::cout << "Success: execution of " << cmd << std::endl;

            //Clean up by waiting / telling the child that it can "destroy itself"
            int status;
            waitpid(process_id, &status, 0); //0 -> no flags here
            return true;
        }
        else if (state == PROCESS_STATE::ERROR)
        {
            kill_process(process_id);
            std::cout << "Error in execution of " << cmd << std::endl;
            return false;
        }

        //Use longer sleep time until short before end of timeout
        time_passed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
        auto remaining_time = static_cast<int64_t>(timeout_seconds) * 1000 - time_passed_ms;
        if (remaining_time > 1000)
        {
            std::this_thread::sleep_for(std::chrono::seconds(1));
        }
        else if (remaining_time > 0)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(remaining_time));
        }
        
        time_passed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::high_resolution_clock::now() - start_time).count();
    }

    //Now kill the process, as it has not yet finished its execution
    //std::cout << "Killing" << std::endl;
    kill_process(process_id);
    std::cout << "Could not execute in time: " << cmd << std::endl;
    return false;
}

//Communication with Pipes, from gnu.org, slightly modified
std::string read_from_pipe (int file)
{
    FILE *stream;
    char c;
    std::stringstream read_stream;

    stream = fdopen (file, "r");
    while ((c = fgetc (stream)) != EOF)
        read_stream << c;
    fclose (stream);

    return read_stream.str();
}

void write_to_pipe (int file, std::string msg)
{
  FILE *stream;
  stream = fdopen (file, "w");
  fprintf (stream, msg.c_str());
  fclose (stream);
}

//! Communication via message queues requires a communication struct
struct CommandMsg {
    //! Always required, can be used as and ID to get a sepcific msg (won't be used here though)
    long mtype;

    //! Better to use only one additional entry, so if more than one is required, define another struct here
    struct CommandInfo {
        char command[500];
        int timeout_seconds;
    } command;
};

/**
 * \brief Create a CommandMsg from a command string and a given timeout. Returns false if creation failed 
 * (usually: due to too large command_string, which is limited by the size of command in CommandInfo)
 * \param command_string The command to execute 
 * \param timeout_seconds Timeout for the command, set to a value <0 to disable timeouts for this command 
 *                      (SYSTEM is used then instead of using fork explicitly)
 */
bool create_msg(std::string command_string, int timeout_seconds, CommandMsg& command_out)
{
    command_out.command.timeout_seconds = timeout_seconds; //In our syntax, this would mean that we do not want a timeout
    command_out.mtype = 1; //Irrelevant for us

    //WARNING: In a real-world scenario, do not forget to make sure that the command string is shorter than the size of
    //command_out.command.command, else we do not get a null terminated C string or need to truncate (which would be undesirable as well)
    std::strncpy(command_out.command.command, command_string.c_str(), sizeof(command_out.command.command));
    command_out.command.command[sizeof(command_out.command.command)-1] = '\0'; //For safety reasons, in case the string is too long

    //Now: Compare the copied string to the original, return false if this failed
    if (std::strcmp(command_out.command.command, command_string.c_str()) != 0) 
        return false;

    return true;
}

/**
 * \brief Send the desired message on the given message queue. Prints an error and returns false if it failed, else returns true
 * \param msqid ID of the msg queue to use
 * \param msg Message to send
 */
bool send_msg(int msqid, CommandMsg& msg)
{
    //Send the message. There might be some problem with it, so do not forget to check for errors. The flag is not used (0)
    auto return_code = msgsnd(msqid, &msg, sizeof(CommandMsg::CommandInfo), 0);
    if (return_code == -1)
    {
        std::cerr << "ERROR: Could not send IPC Message: " << std::strerror(errno) << std::endl;
        return false;
    }

    return true;
}

/**
 * \brief Receive a message on the given message queue. Prints an error and returns false if it failed, else returns true
 * \param msqid ID of the msg queue to use
 * \param msg The received msg is stored here
 */
bool receive_msg(int msqid, CommandMsg& msg)
{
    auto return_code = msgrcv(msqid, &msg, sizeof(CommandMsg::CommandInfo), 0, 0);
    if (return_code == -1)
    {
        std::cerr << "ERROR: Could not receive IPC Message: " << std::strerror(errno) << std::endl;
        return false;
    }

    return true;
}

//In this test scenario, we want to create a parent and a child process
//The parent process tells the child process which other processes to create
//The child process then uses the above functions to do so
int main(int argc, char *argv[]) {
    //TODO: Deal with interrupts / program exit & kill the child process there

    //Alternative for waiting (but without being able to determine if an error occured as in get_child_process_state):
    //This always prevents zombie processes:
    //signal(SIGCHLD, SIG_IGN);
    //Currently, this is not used, instead, we always wait
    //Advantage: We can see if a child process had to exit due to an error
    //Disadvantage: We get zombie processes if our program crashes
    //-> What is more important?

    //Create parent and child process, allow communication via pipe
    //But: It does not seem to be advisable to use pipes for message-communication
    // int command_pipe[2];

    // if (pipe(command_pipe))
    // {
    //     std::cerr << "Pipe creation failed!" << std::endl;
    //     return EXIT_FAILURE;
    // }

    //...
    //We only want to listen, close the pipe's write end
    //close(command_pipe[1]); std::string received_command = read_from_pipe(command_pipe[0]); ...
    //We only want to write, close the pipe's listen end
    //close(command_pipe[0]); write_to_pipe(command_pipe[1], "This is a test"); ...

    //So: We will have to use something else instead
    //Different methods exist for IPC (Inter Process Communication)
    //- FIFOs
    //- Message Queues
    //- File Locking / Semaphores + File Access
    //- Pipes
    //- Signals
    //...
    //Our requirements: 
    //- Send command strings of arbitrary length
    //- Set a timeout or use SYSTEM (so: tell which one of those to use)
    //- Maybe return to the main program if a child execution failed
    //-> Message Queues seem to be the best choice in such a scenario
    //Useful guide: http://beej.us/guide/bgipc/html/single/bgipc.html#fork

    //For now, we will try to use a message queue
    //To get a (hopefully) unqiue ID, we need to create one given a file location
    //For better portability, we hope that argv[0] contains our current file location and use that 
    //(It usually should)#
    //NOTE: We do not use further control or error checking (e.g. in case of a full queue) right now,
    //because due to the desired way of operation we do not expect the queue to become full
    auto key = ftok(argv[0], 'a'); //The char is usually arbitrary, I chose a
    auto msqid = msgget(key, 0666 | IPC_CREAT); //Permissions: rw-rw-rw-

    if (key == -1 || msqid == -1)
    {
        std::cerr << "ERROR: Could not create IPC Message Queue which is required for communicating commands to start external programs!" << std::endl;
        exit(EXIT_FAILURE);
    }

    int process_id = fork();
    if (process_id == 0)
    {
        //Tell the child to set its group process ID to its process ID, or else things like kill(-pid) to kill a ping-while-loop won't work
        setpgid(0, 0);

        //Now try to receive a message; the child waits if there currently is no message present
        //0 to receive the next message on the queue, irrelevant of the value of mtype, flag not used (0 as well)
        //TODO: Create a test scenario where the child gets killed while waiting, to test if that works
        while (true)
        {
            CommandMsg msg;
            receive_msg(msqid, msg);

            std::cout << "Child received command: " << msg.command.command << std::endl;

            //Exit condition - just compare the first characters, as strncpy fills up the char array
            std::string msg_string = msg.command.command;
            if (msg_string.compare(0, 4, "EXIT") == 0)
                break;
        }

        std::cout << "Child is stopping execution..." << std::endl;

        exit(EXIT_SUCCESS);
    }
    else if (process_id > 0)
    {
        //We are in the parent process; simulate work using waiting functions, try to communicate with the child in between

        //Create and send messages
        for (auto i = 0; i < 12; ++i)
        {
            std::stringstream msg_stream;
            msg_stream << "This is test number " << i + 1 << ".";

            CommandMsg msg;
            if (create_msg(msg_stream.str(), -1, msg))
            {
                send_msg(msqid, msg);
            }
            else
            {
                std::cerr << "ERROR: Could not create IPC Message, command string was too large" << std::endl;
            }
        }

        //Send final msg to tell the child to stop its execution
        CommandMsg msg;
        if (create_msg("EXIT", -1, msg))
        {
            send_msg(msqid, msg);
        }
        else
        {
            std::cerr << "ERROR: Could not create IPC Message, command string was too large" << std::endl;
        }

        //----------------------------------------CLEANING UP-----------------------------------------------------------//
        //The parent ALWAYS has to wait for the child to finish, else ressources are not cleaned up
        //There is an alternative to that (ignore SIGCHLD), but as far as I interpret it that would probably lead to 
        //problems with get_child_process_state
        int status;
        waitpid(process_id, &status, 0); //0 -> no flags here

        //Destroy the message queue
        auto return_code = msgctl(msqid, IPC_RMID, NULL);
        if (return_code == -1)
        {
            std::cerr << "ERROR: Could not send IPC Message: " << std::strerror(errno) << std::endl;
            exit(EXIT_FAILURE);
        }

        exit(EXIT_SUCCESS);
    }
    else 
    {
        //We could not spawn a new process - usually, the program should not just break at this point, unless that behaviour is desired
        //TODO: Change behaviour
        std::cerr << "Error when using fork: Could not create 'meta'-child process!";
        exit(EXIT_FAILURE);
    }

    return 0;
}