#include "cameras/cameras.hpp"

rclcpp::QoS make_discover_qos() {
    rclcpp::QoS qos(1);
    qos.reliability(RMW_QOS_POLICY_RELIABILITY_RELIABLE);
    qos.durability(RMW_QOS_POLICY_DURABILITY_TRANSIENT_LOCAL);
    return qos;
}

rclcpp::QoS discover_qos = make_discover_qos();