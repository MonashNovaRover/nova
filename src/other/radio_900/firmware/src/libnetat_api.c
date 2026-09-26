#include "libnetat.h"
#include <string.h>

int netat_wrapper_init(const char *ifname) {
    return libnetat_init((char *)ifname);
}

int netat_wrapper_get_device_count() {
    return libnetat.device_count;
}

void netat_wrapper_get_device(int index, char *dest_mac) {
    memcpy(dest_mac, libnetat.device_list[index], 6);
}

int netat_wrapper_set_target(int index) {
    if (index >= libnetat.device_count) return -1;
    memcpy(libnetat.dest, libnetat.device_list[index], 6);
    return 0;
}

int netat_wrapper_send(const char *cmd, char *resp, int size) {
    return libnetat_send((char *)cmd, resp, size);
}
