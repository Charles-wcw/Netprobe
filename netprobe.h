#ifndef NET_PROBE_H
#define NET_PROBE_H

#include "cross_platform_socket.h"
#include <stdint.h>
#include <unistd.h>

#define MAX_HOST_LEN 256

#define LOCAL_HOST "127.0.0.1"
#define ANY_HOST "0.0.0.0"

typedef enum
{
    SEND = 0,
    RECV,
    HOST
} EMode;

typedef enum
{
    UDP = 0,
    TCP
} EProto;

struct Config
{
    EMode eMode;
    int Stat; // ms
    char Host[MAX_HOST_LEN];
    char rHost[MAX_HOST_LEN];
    uint16_t rPort;
    char lHost[MAX_HOST_LEN];
    uint16_t lPort;
    EProto eProto;
    int pktSize; // bytes
    int pktRate; //  bytes/s
    int pktNum;
    int sBufSize; // bytes
};

void InitConfig(struct Config *pConfig);
void PrintUsage(char *pProgName);
void ParseArguments(struct Config *pConfig, int argc, char **argv);
void RunSend(struct Config *pConfig);
void RunRecv(struct Config *pConfig);
void RunHost(struct Config *pConfig);

#endif // NET_PROBE_H
