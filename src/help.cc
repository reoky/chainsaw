#include "help.h"

// Prints the ASCII art top banner for chainsaw
void print_banner() {
    std::cout << "                              -ohmmy-                                       " << std::endl
            << "                   .:.      :hmmmm+       -+shdddhs-                        " << std::endl
            << "                `+dmo     .ymmmmmm.   `:smmmmmmmh:`                         " << std::endl
            << "               .dmmm-    -dmmmmmmmh..ommmmmmmmmy                            " << std::endl
            << "              `dmmmmh:  :mmmmmmmmmmmmmmmmmmmmmmy  `.:/+oooo+:.              " << std::endl
            << "       `.     smmmmmmmmhmmmmmmmmmmmmmmmmmmmmmmmmhmmmmmmmmmmmmmd+`           " << std::endl
            << "      +d/    `mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmd+:--.           " << std::endl
            << "     ommy    :mmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmmd`                " << std::endl
            << "    `mmmmd+:-ommmmmmmmmmmmmmmhyo+//:///+oyhmmmmmmmmmmmmmmmy--.`             " << std::endl
            << "    `mmmmmmmmmmmmmmmmmmmdo/..:/osyyyyyyso/-../odmmmmmmmmmmmmmmmmdys/.       " << std::endl
            << "    `mmmmmmmmmmmmmmmmd+../ydmdyo+/::::/+oydmdy/..+dmmmmmmmmmmmmmmmmmmdo`    " << std::endl
            << "     ymmmmmmmmmmmmmh:`/hmdo:`              `-ohmh+`-ymmmmmmmmmmmmmmmmmmm-   " << std::endl
            << ".s`    :mmmmmmmmmmmd:`ommo-                      .ommo`-hmmmmmmmmmmmmo:-:+o " << std::endl;
}

// Prints the usage options for chainsaw
void print_usage() {
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "| Chainsaw 0.43 | Split files into shards for easy transport.                  |" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "| Chainsaw is meant to be easy to use, but there are several options you can   |" << std::endl;
    std::cout << "| opt to use. By default, running chainsaw on an ordinary file will break it   |" << std::endl;
    std::cout << "| into eight shards of nearly equal size.                                      |" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "---- OPT ------------------------------------------ USE CASE -------------------" << std::endl;
    std::cout << "|    -d         |  Store shards in a new directory while splitting.            |" << std::endl;
    std::cout << "|               |                                                              |" << std::endl;
    std::cout << "|    -i         |  Display information about a single shard.                   |" << std::endl;
    std::cout << "|               |                                                              |" << std::endl;
    std::cout << "|    -n         |  Named prefix to use while creating folders and shards.      |" << std::endl;
    std::cout << "|               |                                                              |" << std::endl;
    std::cout << "|    -s         |  Specify the maximum size of each shard in MB.               |" << std::endl;
    std::cout << "|               |                                                              |" << std::endl;
    std::cout << "|    -v         |  Enable verbose mode to see what's happening under the hood. |" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
    std::cout << "----------------------------------  EXAMPLES  ----------------------------------" << std::endl;
    std::cout << "| $ chainsaw <file>                    |  Splits the file into eight shards.   |" << std::endl;
    std::cout << "|                                      |                                       |" << std::endl;
    std::cout << "| $ chainsaw <shards>                  |  Join shards back info a file.        |" << std::endl;
    std::cout << "|                                      |                                       |" << std::endl;
    std::cout << "| $ chainsaw -s 100MB -n loves <file>  |  Make 100MB shards named 'loves7.10'  |" << std::endl;
    std::cout << "|                                      |                         (7 out of 10) |" << std::endl;
    std::cout << "| $ chainsaw -d <file>                 |  Store in a default-named directory.  |" << std::endl;
    std::cout << "--------------------------------------------------------------------------------" << std::endl;
}