#ifndef LIBNETAT_H
#define LIBNETAT_H

#define MAX_DEVICES 6

int libnetat_init(char *ifname);
int libnetat_send(char *atcmd, char *resp_buff, int buf_size);

extern struct netat_mgr {
    int sock;
    char dest[6];
    char cookie[6];
    char recvbuf[1024];
    char device_list[MAX_DEVICES][6];
    int device_count;
} libnetat;

static const char desired_mac_order[][6] = {
    {0x4a, 0x06, 0x59, 0x77, 0x92, 0xc8},
    {0x22, 0x9f, 0xf1, 0x9b, 0xa5, 0x28},
    {0x82, 0x59, 0x13, 0xa6, 0x77, 0x28},
    {0x22, 0x9f, 0xf1, 0x77, 0x50, 0xa8},
    {0x1e, 0x05, 0x59, 0x68, 0x7b, 0x18},
    {0x4a, 0x06, 0x59, 0x77, 0x7e, 0xc0},
};
static const int desired_mac_count = sizeof(desired_mac_order) / sizeof(desired_mac_order[0]);

#endif // LIBNETAT_H
