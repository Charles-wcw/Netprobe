#include "netprobe.h"
#include <stdio.h>

int main(int argc, char *argv[])
{
    if (argc < 2)
    {
        PrintUsage(argv[0]);
    }

    struct Config config;

    InitConfig(&config);
    ParseArguments(&config, argc, argv);

    SOCK_INIT(); // Initialize socket library

    switch (config.eMode)
    {
    case SEND:
        RunSend(&config);
        break;
    case RECV:
        RunRecv(&config);
        break;
    case HOST:
        RunHost(&config);
        break;
    default:
        fprintf(stderr, "unknown mode: %d\n", config.eMode);
        break;
    }

    SOCK_CLEAN(); // Clean up socket library

    return 0;
}
