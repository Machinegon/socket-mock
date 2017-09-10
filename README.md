# socket-mock

DESCRIPTION
==================================
A small utility that allows you to listen/reply to incoming TCP socket communication on specified port number.


COMPILATION DIRECTIVES
==================================
Use any compiler with the standard c library, standard c++ library and posix library.

* Clone the project
* Edit the makefile and set CXX to your specific compiler
* Make

USAGE
==================================
The program must be started with the following parameters.

* [--port=p] -> Port number (Must be root under 1080)
* [--ip <ip>] -> Server IP, normally your ethernet interface IP address
* [--file <path>] -> Path to a command/response configuration file


COMMAND/RESPONSE CONFIGURATION FILE
==================================
If a command/response file is set, the server will parse a file to look for recognized commands and answer the related response.
The file must be set up the following way:

[COMMAND] || [RESPONSE]

[COMMAND] || [RESPONSE]

Each line has a command and a corresponding response and the "||" serve as a delimiter. 
Wildcard character "*" might be used to look only for the start or the end of a command.
For example:

*PARTIALCOMMAND||{"jsonResult": "OK"}

PARTIALCOMMAND*||hello world
