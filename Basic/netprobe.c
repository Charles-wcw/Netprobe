#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <sys/time.h>
#include "netprobe.h"

void InitConfig(struct Config *pConfig) {
    pConfig->eMode = SEND;
    pConfig->Stat = 500; // ms
    strncpy(pConfig->rHost, LOCAL_HOST, strlen(LOCAL_HOST));
    pConfig->rPort = 4180;

    strncpy(pConfig->lHost, ANY_HOST, strlen(ANY_HOST));
    pConfig->lPort = 4180;
    pConfig->eProto = UDP;

    pConfig->pktSize = 1000; // bytes
    pConfig->pktRate = 1000; //  bytes/s
    pConfig->pktNum = 0; // infinite
    pConfig->sBufSize = 1000; // bytes
}

void PrintUsage(char *pProgName) {
    fprintf(
        stderr,
        "Usage: %s [-send|-recv|-host hostname] [-stat yyy] [-rhost hostname] [-rport portnum] [-lhost hostname] [-lport portnum] [-proto tcp|udp] [-pktsize bsize] [-pktrate txrate] [-pktnum num] [-sbufsize bsize]\n",
        pProgName);
    exit(EXIT_FAILURE);
}

void ParseArguments(struct Config *pConfig, int argc, char **argv) {
    opterr = 0;
    int opt;
    while (true) {
        static struct option long_options[] = {
            {"send", no_argument, NULL, 's'},
            {"recv", no_argument, NULL, 'r'},
            {"host", required_argument, NULL, 'd'},
            {"stat", required_argument, NULL, 'a'},
            {"rhost", required_argument, NULL, 'b'},
            {"rport", required_argument, NULL, 'c'},
            {"lhost", required_argument, NULL, 'e'},
            {"lport", required_argument, NULL, 'f'},
            {"proto", required_argument, NULL, 'g'},
            {"pktsize", required_argument, NULL, 'i'},
            {"pktrate", required_argument, NULL, 'j'},
            {"pktnum", required_argument, NULL, 'k'},
            {"sbufsize", required_argument, NULL, 'l'},
            {"help", no_argument, NULL, 'h'},
            {0, 0, 0, '?'},
        };
        int option_index = 0;
        opt = getopt_long_only(argc, argv, "", long_options, &option_index);
        if (opt == -1) {
            break;
        }

        switch (opt) {
            case 0:
                break;
            case 's':
                pConfig->eMode = SEND;
                break;
            case 'r':
                pConfig->eMode = RECV;
                break;
            case 'd':
                pConfig->eMode = HOST;
                strncpy(pConfig->Host, optarg, strlen(optarg));
                break;
            case 'a':
                pConfig->Stat = atoi(optarg);
                break;
            case 'b':
                break;
            case 'c':
                pConfig->rPort = atoi(optarg);
                break;
            case 'e':
                strncpy(pConfig->lHost, optarg, strlen(optarg));
                break;
            case 'f':
                pConfig->lPort = atoi(optarg);
                break;
            case 'g':
                if (strcmp(optarg, "udp") == 0) {
                    pConfig->eProto = UDP;
                } else if (strcmp(optarg, "tcp") == 0) {
                    pConfig->eProto = TCP;
                } else {
                    fprintf(stderr, "Invalid protocol\n");
                    exit(EXIT_FAILURE);
                }
                break;
            case 'i':
                pConfig->pktSize = atoi(optarg);
                break;
            case 'j':
                pConfig->pktRate = atoi(optarg);
                break;
            case 'k':
                pConfig->pktNum = atoi(optarg);
                break;
            case 'l':
                pConfig->sBufSize = atoi(optarg);
                break;
            case ':':
            case '?':
            case 'h':
            default: {
                PrintUsage(argv[0]);
            }
        }
    }

    return;
}

void RunSend(struct Config *pConfig) {
    sock_t sock;
    struct sockaddr_in server_addr;
    char *buffer;
    unsigned long seq_num = 0;
    unsigned long packet_count = 0, total_packet_count = 0;
    struct timeval start_time, current_time;
    long elapsed_time, total_elapsed_time = 0;
    double send_delay;

    // Print Config
    printf("Mode             : Send\n");
    printf("Stat             : %d ms\n", pConfig->Stat);
    printf("Remote Host      : %s\n", pConfig->rHost);
    printf("Remote Port      : %d\n", pConfig->rPort);
    printf("Local Host       : %s\n", pConfig->lHost);
    printf("Local Port       : %d\n", pConfig->lPort);
    printf("Protocol         : %s\n", pConfig->eProto == UDP ? "UDP" : "TCP");
    printf("Packet Size      : %d bytes\n", pConfig->pktSize);
    printf("Packet Rate      : %d bytes/s\n", pConfig->pktRate);
    printf("Packet Number    : %d\n", pConfig->pktNum);
    printf("Socket Buffer    : %d bytes\n", pConfig->sBufSize);

    // Create socket
    if (pConfig->eProto == TCP) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(pConfig->rPort);
        inet_pton(AF_INET, pConfig->rHost, &server_addr.sin_addr);
        if (connect(sock, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
            fprintf(stderr, "connect failed\n");
            exit(EXIT_FAILURE);
        }
        // printf("Connected to %s:%d\n", pConfig->rHost, pConfig->rPort);
    } else {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(pConfig->rPort);
        inet_pton(AF_INET, pConfig->rHost, &server_addr.sin_addr);
    }

    // Calculate send delay
    if (pConfig->pktRate > 0) {
        send_delay = (double) pConfig->pktSize / pConfig->pktRate; // seconds
    } else {
        send_delay = 0;
    }

    // Allocate buffer
    buffer = (char *) malloc(pConfig->pktSize);
    if (buffer == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }
    memset(buffer, 'A', pConfig->pktSize);

    // Start time
    gettimeofday(&start_time, NULL);

    // Send loop
    int bytes_sent = 0;
    while (true) {
        *((unsigned long *) buffer) = seq_num; // sequence number
        if (pConfig->eProto == TCP) {
            bytes_sent = send(sock, buffer, pConfig->pktSize, 0);
        } else {
            bytes_sent = sendto(sock, buffer, pConfig->pktSize, 0,
                                (struct sockaddr *) &server_addr, sizeof(server_addr));
        }

        if (bytes_sent < 0) {
            perror("send failed");
            break;
        } else if (bytes_sent == 0) {
            printf("Connection closed\n");
            break;
        }

        packet_count++;
        seq_num++;
        total_packet_count++;

        // Print statistics
        gettimeofday(&current_time, NULL);
        elapsed_time = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                       (current_time.tv_usec - start_time.tv_usec) / 1000; // ms

        if (elapsed_time >= pConfig->Stat) {
            total_elapsed_time += elapsed_time;
            double throughput = (packet_count * pConfig->pktSize * 8.0) / (total_elapsed_time * 1000); // Mbps
            printf("Elapsed [%ld ms] Pkts [%lu] Rate [%.2f Mbps]\n",
                   total_elapsed_time, packet_count, throughput);
            // packet_count = 0;
            gettimeofday(&start_time, NULL);
        }

        if (pConfig->pktNum > 0 && total_packet_count >= pConfig->pktNum) {
            printf("Sent %lu packets and exit\n", total_packet_count);
            break;
        }

        if (send_delay > 0) {
            usleep((int) (send_delay * 1000)); // microseconds
        }
    }

    free(buffer);

    SOCK_CLOSE(sock);
}

void RunRecv(struct Config *pConfig) {
    sock_t sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    char *buffer;
    socklen_t addr_len = sizeof(client_addr);
    int bytes_received;
    unsigned long packet_count = 0;
    unsigned long lost_packets = 0;
    struct timeval start_time, current_time, last_recv_time;
    long elapsed_time, total_elapsed_time = 0;
    unsigned long seq_num = 0;
    double total_jitter = 0.0;
    unsigned long total_intervals = 0;

    // Print Config
    printf("Mode             : Recv\n");
    printf("Stat             : %d ms\n", pConfig->Stat);
    printf("Remote Host      : %s\n", pConfig->rHost);
    printf("Remote Port      : %d\n", pConfig->rPort);
    printf("Local Host       : %s\n", pConfig->lHost);
    printf("Local Port       : %d\n", pConfig->lPort);
    printf("Protocol         : %s\n", pConfig->eProto == UDP ? "UDP" : "TCP");
    printf("Packet Size      : %d bytes\n", pConfig->pktSize);
    printf("Packet Rate      : %d bytes/s\n", pConfig->pktRate);
    printf("Packet Number    : %d\n", pConfig->pktNum);
    printf("Socket Buffer    : %d bytes\n", pConfig->sBufSize);

    // Create socket
    if (pConfig->eProto == TCP) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(pConfig->lPort);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
        listen(sock, 5);
        client_sock = accept(sock, (struct sockaddr *) &client_addr, &addr_len);
        if (client_sock < 0) {
            fprintf(stderr, "accept failed\n");
            SOCK_CLEAN();
            exit(EXIT_FAILURE);
        }
        printf("Accepted connection from %s:%d\n", inet_ntoa(client_addr.sin_addr), ntohs(client_addr.sin_port));
    } else {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(pConfig->lPort);
        server_addr.sin_addr.s_addr = INADDR_ANY;
        bind(sock, (struct sockaddr *) &server_addr, sizeof(server_addr));
    }

    // Allocate buffer
    buffer = (char *) malloc(pConfig->pktSize);
    if (buffer == NULL) {
        fprintf(stderr, "malloc failed\n");
        exit(EXIT_FAILURE);
    }

    // Start time
    gettimeofday(&start_time, NULL);
    gettimeofday(&last_recv_time, NULL);

    // Receive loop
    while (true) {
        if (pConfig->eProto == TCP) {
            bytes_received = recv(client_sock, buffer, pConfig->pktSize, 0);
        } else {
            bytes_received = recvfrom(sock, buffer, pConfig->pktSize, 0,
                                      (struct sockaddr *) &client_addr, &addr_len);
        }

        if (bytes_received < 0) {
            perror("recv failed");
            break;
        } else if (bytes_received == 0) {
            printf("Connection closed\n");
            break;
        }

        packet_count++;
        gettimeofday(&current_time, NULL);

        // Calculate jitter
        if (packet_count > 1) {
            double recv_interval = (current_time.tv_sec - last_recv_time.tv_sec) * 1000 +
                                   (current_time.tv_usec - last_recv_time.tv_usec) / 1000.0; // ms
            double jitter = fabs(recv_interval - total_jitter / (total_intervals > 0 ? total_intervals : 1));
            total_jitter += jitter;
            total_intervals++;
        }

        last_recv_time = current_time; // update last recv time

        // Calculate lost packets
        unsigned long recv_seq_num = *((unsigned long *) buffer);
        if (recv_seq_num > seq_num) {
            lost_packets += (recv_seq_num - seq_num - 1);
            seq_num = recv_seq_num;
        } else {
            seq_num++;
        }

        // Print statistics
        gettimeofday(&current_time, NULL);
        elapsed_time = (current_time.tv_sec - start_time.tv_sec) * 1000 +
                       (current_time.tv_usec - start_time.tv_usec) / 1000; // ms

        if (elapsed_time >= pConfig->Stat) {
            total_elapsed_time += elapsed_time;
            double loss_rate = (double) lost_packets / (packet_count + lost_packets) * 100.0;
            double avg_jitter = total_intervals > 0 ? total_jitter / total_intervals : 0.0;
            printf("Elapsed [%ld ms] Pkts [%lu] Lost [%lu, %.2f%%] Jitter [%.2f ms]\n",
                   total_elapsed_time, packet_count, lost_packets, loss_rate, avg_jitter);
            gettimeofday(&start_time, NULL);
            // packet_count = 0;
            // lost_packets = 0;
            // total_jitter = 0.0;
            // total_intervals = 0;
        }
    }

    free(buffer);
    SOCK_CLOSE(sock);
}

void RunHost(struct Config *pConfig) {
    if (strlen(pConfig->Host) == 0) {
        snprintf(pConfig->Host, MAX_HOST_LEN, "%s", LOCAL_HOST);
    }

    struct hostent *host_info = gethostbyname(pConfig->Host);
    if (host_info == NULL) {
        fprintf(stderr, "gethostbyname failed\n");
        exit(EXIT_FAILURE);
    }

    printf("Hostname         : %s\n", pConfig->Host);
    printf("Official name    : %s\n", host_info->h_name);
    printf("Address type     : %d\n", host_info->h_addrtype);
    printf("Address length   : %d\n", host_info->h_length);

    struct in_addr **addr_list = (struct in_addr **) host_info->h_addr_list;
    for (int i = 0; addr_list[i] != NULL; i++) {
        printf("Address %d        : %s\n", i + 1, inet_ntoa(*addr_list[i]));
    }
}
