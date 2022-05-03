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
        "\n  -x, --debug         \tDebug mode - prints additional info to log");
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
        "\n  --daemon            \tUsed to launch initial daemon process to "
        "monitor traffic");
    printf("\nCLI Arguments:\n");
    printf("If no arguments provided, will default to 1 day timeframe.\n");
    printf(
        "\n  -d, --days [int]    \tSpecify how many past days to sum "
        "application traffic usage for.");
    printf(
        "\n  -h, --hours [int]   \tSpecify how many past hours to sum "
        "application traffic usage for.");
    printf(
        "\n  -m, --minutes [int] \tSpecify how many past minutes to sum "
        "application traffic usage for.");
    printf(
        "\n  --sort [sort]    \tSpecify the sort preference for table; "
        "Descending based on either \"rx\" or \"tx\". Default: rx");
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
    args->time = {0, 0, 0};
    args->sort = RX_DESC;

    bool timeframe_set = false;

    const std::vector<std::string_view> arg_list(argv + 1, argv + argc);
    for (auto it = arg_list.begin(), end = arg_list.end(); it != end; ++it) {
        std::string_view arg{*it};

        if (arg == "--help") {
            print_help();
            exit(1);
        }

        if (arg == "--daemon") {
            args->daemon = true;
        }

        if (arg == "-x" || arg == "--debug") {
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

        if (arg == "-d" || arg == "--days") {
            if (it + 1 != end) {
                try {
                    args->time.days = std::stoi(std::string(*(it + 1)));
                } catch (const std::invalid_argument &ia) {
                    fprintf(stderr,
                            "The days argument (-d, --days) requires an "
                            "integer. Invalid argument: %s\n",
                            ia.what());
                    exit(1);
                }
            } else {
                fprintf(stderr,
                        "The days argument (-d, --days) requires an "
                        "integer.\n");
                exit(1);
            }

            timeframe_set = true;
        }

        if (arg == "-h" || arg == "--hours") {
            if (it + 1 != end) {
                try {
                    args->time.hours = std::stoi(std::string(*(it + 1)));
                } catch (const std::invalid_argument &ia) {
                    fprintf(stderr,
                            "The hours argument (-h, --hours) requires an "
                            "integer. Invalid argument: %s\n",
                            ia.what());
                    exit(1);
                }
            } else {
                fprintf(stderr,
                        "The hours argument (-h, --hours) requires an "
                        "integer.\n");
                exit(1);
            }

            timeframe_set = true;
        }

        if (arg == "-m" || arg == "--minutes") {
            if (it + 1 != end) {
                try {
                    args->time.minutes = std::stoi(std::string(*(it + 1)));
                } catch (const std::invalid_argument &ia) {
                    fprintf(stderr,
                            "The days argument (-m, --minutes) requires an "
                            "integer. Invalid argument: %s\n",
                            ia.what());
                    exit(1);
                }
            } else {
                fprintf(stderr,
                        "The days argument (-m, --minutes) requires an "
                        "integer.\n");
                exit(1);
            }

            timeframe_set = true;
        }

        if (arg == "--sort") {
            if (it + 1 != end) {
                std::string_view pref = *(it + 1);

                if (pref == "tx" || pref == "TX")
                    args->sort = TX_DESC;
                else if (pref == "rx" || pref == "RX")
                    args->sort = RX_DESC;
                else {
                    fprintf(stderr,
                            "The sort argument (--sort) requires "
                            "a string specifing a sorting preference. Options: "
                            "rx, tx. Example: --sort tx\n");
                    exit(1);
                }
            } else {
                fprintf(stderr,
                        "The sort argument (--sort) requires "
                        "a string specifing a sorting preference. Options: "
                        "rx, tx. Example: --sort tx\n");
                exit(1);
            }
        }
    }

    if (!timeframe_set) {
        /* If days, hours, or minutes have not been set, default to 1 day */
        args->time = {1, 0, 0};
    }

    return 0;
}

