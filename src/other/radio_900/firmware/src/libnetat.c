#include "libnetat.h"
#include "constants.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <net/if.h>

#define NETAT_BUFF_SIZE (1024)
#define NETAT_PORT      (56789)
#define MAX_SCAN_ATTEMPTS 5

enum WNB_NETAT_CMD {
    WNB_NETAT_CMD_SCAN_REQ = 1,
    WNB_NETAT_CMD_SCAN_RESP,
    WNB_NETAT_CMD_AT_REQ,
    WNB_NETAT_CMD_AT_RESP,
};

struct wnb_netat_cmd {
    char  cmd;
    char  len[2];
    char  dest[6];
    char  src[6];
    char  data[0];
};

struct netat_mgr libnetat;

static int find_mac_index(const char mac[6]) {
    for (int i = 0; i < desired_mac_count; i++) {
        if (memcmp(mac, desired_mac_order[i], 6) == 0) {
            return i;
        }
    }
    return -1;
}

static int compare_mac_custom(const void *a, const void *b) {
    const char *mac_a = a;
    const char *mac_b = b;

    int idx_a = find_mac_index(mac_a);
    int idx_b = find_mac_index(mac_b);

    if (idx_a >= 0 && idx_b >= 0) {
        return idx_a - idx_b;
    } else if (idx_a >= 0) {
        return -1;
    } else if (idx_b >= 0) {
        return 1;
    }

    return memcmp(mac_a, mac_b, 6);
}

static void random_bytes(char *buff, int len) {
    int fd = open("/dev/urandom", O_RDONLY);
    if (fd >= 0) {
        if (read(fd, buff, len) == len) {
            close(fd);
            return;
        }
        close(fd);
    }
    for (int i = 0; i < len; i++)
        buff[i] = (char)(rand() & 0xff);
}

static int sock_send(int sock, char *data, int len) {
    struct sockaddr_in dest = {0};
    dest.sin_family = AF_INET;
    dest.sin_addr.s_addr = INADDR_BROADCAST;
    dest.sin_port = htons(NETAT_PORT);
    return sendto(sock, data, len, 0, (struct sockaddr *)&dest, sizeof(dest));
}

static int sock_recv(int sock, struct sockaddr_in *dest, char *data, int len, int tmo) {
    fd_set rfd;
    struct timeval timeout = { .tv_sec = 0, .tv_usec = tmo * 1000 };
    FD_ZERO(&rfd);
    FD_SET(sock, &rfd);
    int ret = select(sock + 1, &rfd, NULL, NULL, &timeout);
    if (ret > 0 && FD_ISSET(sock, &rfd))
        return recvfrom(sock, data, len, 0, (struct sockaddr *)dest, (socklen_t[]){sizeof(struct sockaddr_in)});
    return 0;
}

static void netat_scan(void) {
    struct wnb_netat_cmd scan = {0};
    random_bytes(libnetat.cookie, 6);
    scan.cmd = WNB_NETAT_CMD_SCAN_REQ;
    memset(scan.dest, 0xff, 6);
    memcpy(scan.src, libnetat.cookie, 6);
    sock_send(libnetat.sock, (char *)&scan, sizeof(scan));
}

static void netat_send(char *atcmd) {
    unsigned short len = htons(strlen(atcmd));
    struct wnb_netat_cmd *cmd = malloc(strlen(atcmd) + sizeof(struct wnb_netat_cmd));
    if (!cmd) return;

    random_bytes(libnetat.cookie, 6);
    memset(cmd, 0, sizeof(struct wnb_netat_cmd));
    cmd->cmd = WNB_NETAT_CMD_AT_REQ;
    memcpy(cmd->len, &len, 2);
    memcpy(cmd->dest, libnetat.dest, 6);
    memcpy(cmd->src, libnetat.cookie, 6);
    memcpy(cmd->data, atcmd, strlen(atcmd));

    sock_send(libnetat.sock, (char *)cmd, strlen(atcmd) + sizeof(struct wnb_netat_cmd));
    free(cmd);
}

static int netat_recv(char *buff, int len, int tmo) {
    int ret, off = 0;
    struct sockaddr_in from;
    struct wnb_netat_cmd *cmd;

    do {
        memset(libnetat.recvbuf, 0, NETAT_BUFF_SIZE);
        ret = sock_recv(libnetat.sock, &from, libnetat.recvbuf, NETAT_BUFF_SIZE, tmo);
        if (ret >= sizeof(struct wnb_netat_cmd)) {
            cmd = (struct wnb_netat_cmd *)libnetat.recvbuf;
            if (memcmp(cmd->dest, libnetat.cookie, 6) == 0) {
                switch (cmd->cmd) {
                    case WNB_NETAT_CMD_SCAN_RESP:
                        if (libnetat.device_count < MAX_DEVICES) {
                            int known = 0;
                            for (int i = 0; i < libnetat.device_count; i++) {
                                if (memcmp(libnetat.device_list[i], cmd->src, 6) == 0) {
                                    known = 1;
                                    break;
                                }
                            }
                            if (!known) {
                                memcpy(libnetat.device_list[libnetat.device_count], cmd->src, 6);
                                libnetat.device_count++;
                            }
                        }
                        break;

                    case WNB_NETAT_CMD_AT_RESP:
                        if (buff) {
                            strncpy(buff + off, cmd->data, ret - sizeof(struct wnb_netat_cmd));
                            off += (ret - sizeof(struct wnb_netat_cmd));
                        } else {
                            printf("%s\r\n", cmd->data);
                        }
                        break;
                }
            }
        }
    } while (ret > 0);
    if (buff) buff[off] = 0;
    return off;
}

int libnetat_init(char *ifname) {
    int on = 1;
    struct sockaddr_in local_addr = {0};
    struct ifreq req;

    srand(time(NULL));

    memset(libnetat.dest, 0xff, 6);
    libnetat.device_count = 0;
    libnetat.sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (libnetat.sock < 0) return -1;

    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    local_addr.sin_port = htons(NETAT_PORT);

    if (setsockopt(libnetat.sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on)) < 0) goto fail;
    strncpy(req.ifr_name, ifname, IFNAMSIZ);
    if (setsockopt(libnetat.sock, SOL_SOCKET, SO_BINDTODEVICE, &req, sizeof(req)) < 0) goto fail;
    if (bind(libnetat.sock, (struct sockaddr *)&local_addr, sizeof(local_addr)) < 0) goto fail;

    for (int i = 0; i < MAX_SCAN_ATTEMPTS && libnetat.device_count < MAX_DEVICES; i++) {
        netat_scan();
        netat_recv(NULL, 0, 1000);
    }

    // Sort devices by MAC address
    qsort(libnetat.device_list, libnetat.device_count, 6, compare_mac_custom);

    return 0;

fail:
    close(libnetat.sock);
    libnetat.sock = 0;
    return -1;
}

int libnetat_send(char *atcmd, char *resp_buff, int buf_size) {
    if (libnetat.sock == 0) return -1;
    if (libnetat.dest[0] & 0x1) {
        for (int i = 0; i < MAX_SCAN_ATTEMPTS && libnetat.device_count < MAX_DEVICES; i++) {
            netat_scan();
            netat_recv(NULL, 0, 100);
        }
    }
    if (libnetat.dest[0] & 0x1) return -1;
    netat_send(atcmd);
    return netat_recv(resp_buff, buf_size, 10);
}

static int compare_mac(const void *a, const void *b) {
    return memcmp(a, b, 6);
}
