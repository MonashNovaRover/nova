#include <string>
#include <gst/gst.h>
#include "properties/h264.hpp"

void set_h264payload(GstElement* payload, const bool payload_quirk) {
    if (payload_quirk) {
        // Apply patch for gc2093
        g_object_set(payload,
            "aggregate-mode", 1,
            "config-interval", -1,
            NULL);
    }
}

void set_h264parse(GstElement* parse, const int interval) {
    g_object_set(parse,
        "config-interval", interval,
        NULL);
}
