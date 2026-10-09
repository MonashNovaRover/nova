#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "libnetat.h"

#define MAC2STR(a) (a)[0]&0xff, (a)[1]&0xff, (a)[2]&0xff, (a)[3]&0xff, (a)[4]&0xff, (a)[5]&0xff
#define MACSTR     "%02x:%02x:%02x:%02x:%02x:%02x"

int main(int argc, char *argv[]) {
    char values[4][1024];

    if (argc != 2) {
        printf("please input interface name!\n");
        return -1;
    }
    
    if (libnetat_init(argv[1])) {
      printf("libnetat init fail, interface: %s\n", argv[1]);
      return -1;
    }
    
    sleep(1); // wait for device

    printf("Discovered %d device(s):\n", libnetat.device_count);
    for (int i = 0; i < libnetat.device_count; i++) {
        printf("  [%d] " MACSTR "\n", i, MAC2STR(libnetat.device_list[i]));
    }

    while (1) {
        
        for (int i = 0; i < libnetat.device_count; i++) {
            memcpy(libnetat.dest, libnetat.device_list[i], 6);

            libnetat_send("AT+RX_ERR=?",  values[0], sizeof(values[0]));
            libnetat_send("AT+RX_EVM=?",  values[1], sizeof(values[1]));
            libnetat_send("AT+RX_RSSI=?", values[2], sizeof(values[2]));
            libnetat_send("AT+RSSI=?",    values[3], sizeof(values[3]));

            printf(MACSTR " => RX_ERR: %s\tRX_EVM: %s\tRX_RSSI: %s\tRSSI: %s\n",
                   MAC2STR(libnetat.device_list[i]),
                   values[0], values[1], values[2], values[3]);
        }
    }

    return 0;
}
