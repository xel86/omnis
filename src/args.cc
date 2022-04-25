#include "args.h"

#include <cstdio>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

void print_help() {
    printf("Usage: omnis [OPTIONS]...");
    printf("\nNetwork monitoring daemon & cli application.");
    printf("\nExample: omnis --daemon --interval 10");
    printf("\nDaemon Arguments:\n");
    printf(
        "\n  -d, --debug         \tDebug mode - prints additional info to log");
    printf(
        "\n  -v, --verbose       \tVerbose mode - prints even more info to "
        "log");
    printf(
        "\n  -i, --interval [int]\tInterval in seconds to update database "
        "with buffered traffic,");
    printf(
        "\n                        the smaller the number the more space "
        "database with use. Default: 5");
    printf(
        "\n\n  --daemon            \tUsed to launch initial daemon process to "
        "monitor traffic");
}

int parse_args(int argc, char **argv, struct args *args) {
    if (argc > 16) {
        fprintf(stderr, "Entered way too many command line arguments.\n");
        exit(1);
    }

    /* Defaults Arguments */
    args->daemon = false;
    args->debug = false;
    args->verbose = false;
    args->interval = 5;

    const std::vector<std::string_view> arg_list(argv + 1, argv + argc);
    for (auto it = arg_list.begin(), end = arg_list.end(); it != end; ++it) {
        std::string_view arg{*it};

        if (arg == "-h" || arg == "--help") {
            print_help();
            exit(1);
        }

        if (arg == "--daemon") {
            args->daemon = true;
        }

        if (arg == "-d" || arg == "--debug") {
            args->debug = true;
        }

        if (arg == "-v" || arg == "--verbose") {
            args->debug = true;
            args->verbose = true;
        }

        if (arg == "-i" || arg == "--interval") {
            if (it + 1 != end) {
                try {
                    args->interval = std::stoi(std::string(*(it + 1)));
                } catch (const std::invalid_argument &ia) {
                    fprintf(
                        stderr,
                        "The interval argument (-i, --interval) requires an "
                        "integer in seconds. Invalid argument: %s\n",
                        ia.what());
                    exit(1);
                }
            } else {
                fprintf(stderr,
                        "The interval argument (-i, --interval) requires an "
                        "integer in seconds.\n");
                exit(1);
            }
        }
    }

    return 0;
}

