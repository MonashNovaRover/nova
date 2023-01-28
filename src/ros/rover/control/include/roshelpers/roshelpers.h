#pragma once

/*
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

ROS2 helper macros for other nodes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	 control
AUTHOR(S):   Jory Braun
CREATION:	 26/09/2022
EDITED:		 28/09/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
TODO:
 - 
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
*/

using std::placeholders::_1;
using std::placeholders::_2;

#define ROS2_INIT_SUBSCRIPTION(NODE_PTR, SUBSCRIPTION, TOPIC, MESSAGE_TYPE, QOS, CALLBACK_PTR) \
    SUBSCRIPTION = NODE_PTR->create_subscription<MESSAGE_TYPE>( \
        TOPIC, \
        QOS, \
        std::bind(CALLBACK_PTR, NODE_PTR, _1) \
    );

#define ROS2_INIT_SUBSCRIPTION_WITH_DEADLINE(NODE_PTR, SUBSCRIPTION, TOPIC, MESSAGE_TYPE, QOS, CALLBACK_PTR, DEADLINE_CB_PTR) \
    rclcpp::SubscriptionOptionsWithAllocator<std::allocator<void>> options; \
    options.event_callbacks.deadline_callback = [NODE_PTR](rclcpp::QOSDeadlineRequestedInfo) -> void{ \
        (NODE_PTR->*DEADLINE_CB_PTR)(); \
    }; \
    SUBSCRIPTION = NODE_PTR->create_subscription<MESSAGE_TYPE>( \
        TOPIC, \
        QOS, \
        std::bind(CALLBACK_PTR, NODE_PTR, _1), \
        options \
    );

#define ROS2_INIT_PUBLISHER(NODE_PTR, PUBLISHER, TIMER, TOPIC, MESSAGE_TYPE, QOS, TIMER_PERIOD, CALLBACK_PTR) \
    TIMER = NODE_PTR->create_wall_timer(TIMER_PERIOD, std::bind(CALLBACK_PTR, NODE_PTR)); \
    PUBLISHER = NODE_PTR->create_publisher<MESSAGE_TYPE>(TOPIC, QOS);

#define ROS2_INIT_SERVICE_SERVER() \
    SERVICE = NODE_PTR->create_service<MESSAGE_TYPE>(TOPIC, std::bind(CALLBACK_PTR, NODE_PTR, _1, _2));
