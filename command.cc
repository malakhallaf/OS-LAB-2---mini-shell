#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <time.h>
#include <glob.h>
#include <wordexp.h>
#include <vector>
#include <string>
#include <cctype>
#include <iostream>
#include <sstream>
#include "command.h"
#include "tokenizer.h"

void ignoreSignal (int sigNum)
{
    const char msg [] = "\ncannot perform action. use exit to terminate.\nmyshell>";
    write(1, msg, sizeof(msg) - 1);
    std::cin.clear();

}

void childLogger (int child)
{
    int log = open("child_log.log", O_WRONLY|O_CREAT|O_APPEND, 0666);
    if (log >= 0)
    {
        write(log, "CHILD PROCESS TERMINATED!\n", 26);
        close(log);
    }
}

void parse(std::vector<Token> &tokens)
{
    Command::_currentSimpleCommand = new SimpleCommand(); // use a static variable instead of a local one that will be destroyed as soon as parse ends
    
    for (size_t i = 0; i < tokens.size(); i++)
    {
        if (tokens[i].type == TOKEN_EOF)
        {
            break;
        }

        if (tokens[i].type == TOKEN_COMMAND || tokens[i].type == TOKEN_ARGUMENT)
        {
            if (i > 0 && tokens[i-1].type == TOKEN_REDIRECT)
            {
                Command::_currentCommand._outFile = strdup(tokens[i].value.c_str());
            }
            else if (i > 0 && tokens[i-1].type == TOKEN_APPEND)
            {
                Command::_currentCommand._outFile = strdup(tokens[i].value.c_str());
                Command::_currentCommand._append = 1;
            }
            else if (i > 0 && tokens[i-1].type == TOKEN_INPUT)
            {
                Command::_currentCommand._inputFile = strdup(tokens[i].value.c_str());
            }
            else if (i > 0 && tokens[i-1].type == TOKEN_ERROR)
            {
                Command::_currentCommand._errFile = strdup(tokens[i].value.c_str());
            }
            else if (i > 0 && tokens[i-1].type == TOKEN_REDIRECT_AND_ERROR)
            {
                Command::_currentCommand._outFile = strdup(tokens[i].value.c_str());
                Command::_currentCommand._errFile = strdup(tokens[i].value.c_str());
                Command::_currentCommand._out_error = 1;
            }
            else
            {
                Command::_currentSimpleCommand->insertArgument(strdup(tokens[i].value.c_str()));
            }
        }
        else if (tokens[i].type == TOKEN_PIPE)
        {
            Command::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
            Command::_currentSimpleCommand = new SimpleCommand;
        }
        else if (tokens[i].type == TOKEN_BACKGROUND)
        {
            Command::_currentCommand._background = 1;
        }
    }

    if (Command::_currentSimpleCommand->_numberOfArguments > 0)
    {
        Command::_currentCommand.insertSimpleCommand(Command::_currentSimpleCommand);
    }

    Command::_currentCommand.execute();
}

SimpleCommand::SimpleCommand()
{
    _numberOfAvailableArguments = 5;
    _numberOfArguments = 0;
    _arguments = (char **)malloc(_numberOfAvailableArguments * sizeof(char *));
}

void SimpleCommand::insertArgument(char *argument)
{
    if (strchr(argument, '*') != NULL || strchr(argument, '?') != NULL)
    {
        glob_t result;
        glob(argument, GLOB_NOCHECK, NULL, &result);
        for (int i = 0; i < result.gl_pathc; i++)
        {
            if (_numberOfAvailableArguments == _numberOfArguments + 1)
            {
                _numberOfAvailableArguments *= 2;
                _arguments = (char **)realloc(_arguments, _numberOfAvailableArguments * sizeof(char *));
            }

            _arguments[_numberOfArguments] = strdup(result.gl_pathv[i]);
            _arguments[_numberOfArguments + 1] = NULL;

            _numberOfArguments++;
        }
        globfree(&result);
    }
    else
    {
        if (_numberOfAvailableArguments == _numberOfArguments + 1)
        {
            _numberOfAvailableArguments *= 2;
            _arguments = (char **)realloc(_arguments, _numberOfAvailableArguments * sizeof(char *));
        }

        _arguments[_numberOfArguments] = argument;
        _arguments[_numberOfArguments + 1] = NULL;

        _numberOfArguments++;
    }
}

Command::Command()
{
    _numberOfAvailableSimpleCommands = 1;
    _simpleCommands = (SimpleCommand **)
        malloc(_numberOfAvailableSimpleCommands * sizeof(SimpleCommand *)); // should use available capacity variable instead of the variable that has no value assigned to it yet

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _out_error = 0;
    _append = 0;
}

void Command::insertSimpleCommand(SimpleCommand *simpleCommand)
{
    if (_numberOfAvailableSimpleCommands == _numberOfSimpleCommands)
    {
        _numberOfAvailableSimpleCommands *= 2;
        _simpleCommands = (SimpleCommand **)realloc(_simpleCommands, _numberOfAvailableSimpleCommands * sizeof(SimpleCommand *));
    }

    _simpleCommands[_numberOfSimpleCommands] = simpleCommand;
    _numberOfSimpleCommands++;
}

void Command::clear()
{
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            free(_simpleCommands[i]->_arguments[j]);
        }

        free(_simpleCommands[i]->_arguments);
        free(_simpleCommands[i]);
    }

    if (_outFile)
    {
        free(_outFile);
    }

    if (_inputFile)
    {
        free(_inputFile);
    }

    if (_errFile)
    {
        free(_errFile);
    }

    _numberOfSimpleCommands = 0;
    _outFile = 0;
    _inputFile = 0;
    _errFile = 0;
    _background = 0;
    _out_error = 0;
    _append = 0;
}

void Command::print()
{
    printf("\n\n");
    printf("              COMMAND TABLE                \n");
    printf("\n");
    printf("  #   Simple Commands\n");
    printf("  --- ----------------------------------------------------------\n");

    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        printf("  %-3d ", i);
        for (int j = 0; j < _simpleCommands[i]->_numberOfArguments; j++)
        {
            printf("\"%s\" \t", _simpleCommands[i]->_arguments[j]);
        }
    }

    printf("\n\n");
    printf("  Output       Input        Error        Err&Out       Background\n");
    printf("  ------------ ------------ ------------ ------------ ------------\n");
    printf("  %-12s %-12s %-12s %-12s %-12s\n", _outFile ? _outFile : "default",
           _inputFile ? _inputFile : "default", _errFile ? _errFile : "default", _out_error == 1 ? _errFile : "default" // _out_error wasn't even initialized, fixed
           ,_background ? "YES" : "NO");
    printf("\n\n");
}

void Command::execute()
{
    if (_numberOfSimpleCommands == 0)
    {
        return;
    }

    if (_simpleCommands[0]->_numberOfArguments == 0 || _simpleCommands[0] == NULL)
    {
        clear();
        return;
    }

    print();

    if (strcmp(_simpleCommands[0]->_arguments[0],"exit") == 0)
    {
        exit(0);
    }
    else if (strcmp(_simpleCommands[0]->_arguments[0],"cd") == 0)
    {
        if (_simpleCommands[0]->_numberOfArguments == 2)
        {
            if (chdir(_simpleCommands[0]->_arguments[1]) < 0)
            {
                perror("cd");
            }
            clear();
            return;
        }
        else
        {
            if (chdir(getenv("HOME")) < 0)
            {
                perror("cd");
            }
            clear();
            return;
        }

    }

    int input_tracker = 0;
    pid_t lastPID = 0;
    
    for (int i = 0; i < _numberOfSimpleCommands; i++)
    {
        int output_tracker;

        int pipeArray[2];

        if (i == _numberOfSimpleCommands - 1)
        {
            output_tracker = 1;
        }
        else
        {
            pipe(pipeArray);
            output_tracker = pipeArray[1]; 
        }

        pid_t pid = fork();

        if (pid < 0)
        {
            perror("fork");
        }
        else if (pid == 0) // child
        {
            signal(SIGINT, SIG_DFL);

            int fd;

            dup2(input_tracker, 0);
            dup2(output_tracker, 1);

            if (i != _numberOfSimpleCommands - 1)
            {
                close(pipeArray[0]);
            }

            if (input_tracker != 0)
            {
                close(input_tracker);
            }

            if (output_tracker != 1)
            {
                close(output_tracker);
            }

            if (i == 0 && _inputFile != NULL)
            {
                fd = open(_inputFile, O_RDONLY);

                if (fd < 0)
                {
                    perror("open input file");
                    exit(1);
                }

                dup2(fd, 0);
                close(fd);
            }

            if (i == _numberOfSimpleCommands - 1 && _outFile != NULL)
            {
                if (_append == 1)
                {
                    fd = open(_outFile, O_WRONLY|O_CREAT|O_APPEND, 0666);

                    if (fd < 0)
                    {
                        perror("open output file");
                        exit(1);
                    }
                }
                else
                {
                    fd = open(_outFile, O_WRONLY|O_CREAT|O_TRUNC, 0666);

                    if (fd < 0)
                    {
                        perror("open output file");
                        exit(1);
                    }
                }
                dup2(fd, 1);
                close(fd);
            }

            if (i == _numberOfSimpleCommands - 1 && _errFile != NULL)
            {
                fd = open (_errFile, O_WRONLY|O_CREAT|O_TRUNC, 0666);

                if (fd < 0)
                {
                    perror("open error file");
                    exit(1);
                }

                dup2(fd, 2);
                close(fd);
            }

            if (_simpleCommands[i]->_arguments[0] == NULL)
            {
                exit(1);
            }

            execvp(_simpleCommands[i]->_arguments[0], _simpleCommands[i]->_arguments);
            perror("execvp");
            exit(1);
        }
        else // parent
        {
            lastPID = pid;

            if (input_tracker !=0)
            {
                close(input_tracker);
            }

            if (output_tracker != 1)
            {
                close(output_tracker);
            }

            if (i != _numberOfSimpleCommands - 1)
            {
                input_tracker = pipeArray[0];
            }
        }
    }
    if (!_background)
        {
            waitpid(lastPID, NULL, 0);
        }
    clear();
    // prompt() here would make prog crash, while(true) loop alr handles asking for next input
}

void Command::prompt()
{
    std::string input;
    while (true)
    {
        printf("myshell>");
        fflush(stdout);
        std::getline(std::cin, input); 
        std::vector<Token> tokens = tokenize(input);
        parse(tokens);
    }
}

Command Command::_currentCommand;
SimpleCommand *Command::_currentSimpleCommand;

int main()
{
    signal(SIGINT, ignoreSignal);
    signal(SIGCHLD, childLogger);
    Command::_currentCommand.prompt();
    return 0;
}