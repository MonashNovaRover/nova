//
// Created by ecthelion on 6/02/23.
//
#include "blcmd.h"
#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <vector>
#include <numeric>
#include "rclcpp/rclcpp.hpp"

using namespace std;
using namespace org::jcan;

class BLCMDSpoof {
private:
    double velocity;
    double position;
    bool pos_ctrl;
    int id;
    int pos_direction;
    chrono::milliseconds send_rate;
    double target;
    shared_ptr<org::jcan::Bus> bus;

    int16_t convert_to_int16(const double value) {
        // Convert the value to an integer
        return (int16_t) (value * 32767.0f);
    }

    int16_t from_bytes(uint8_t *bytes) {
        return (bytes[0] << 8) | bytes[1];
    }

    double int16_bytes_to_double(uint8_t *bytes) {
        // Scale the value to a double
        return from_bytes(bytes) / 32767.0;
    }

    double uint16_bytes_to_double(uint8_t *bytes) {
        // Scale the value to a double
        return from_bytes(bytes) / 65535.0;
    }

    uint16_t make_can_id(BLCMDSendCommand command) {
        return SEND << 8 | id << 4 | command;
    }

    uint16_t make_can_id(BLCMDReceiveCommand command) {
        return RECEIVE << 8 | id << 4 | command;
    }

    uint16_t make_can_id(TelemetryPacket packet) {
        return RECEIVE << 8 | id << 4 | packet;
    }

public:
    BLCMDSpoof(bool pos_ctrl, int id, chrono::milliseconds send_rate) : pos_ctrl(pos_ctrl), id(id),
                                                                        send_rate(send_rate) {
        target = 0;
        velocity = 0;
        position = 0;
        pos_direction = 0;
        cout << "Creating BLCMDSim with id: " << id << endl;
        bus = org::jcan::new_bus();


        bus->set_id_filter({make_can_id(DRIVE_POSITION), make_can_id(DRIVE_VELOCITY)});
        cout << "Set ID filter" << endl;
        bus->add_callback_to(make_can_id(DRIVE_VELOCITY), this, &BLCMDSpoof::handle_frame);
        cout << "Added velocity callback" << endl;
        bus->add_callback_to(make_can_id(DRIVE_POSITION), this, &BLCMDSpoof::handle_frame);
        cout << "Added position callback" << endl;
        bus->open("vcan0");
        cout << "Opened bus" << endl;


    }

    void handle_frame(Frame frame) {
        if (frame.id == make_can_id(DRIVE_VELOCITY)) {
            velocity = int16_bytes_to_double(&frame.data[0]) * max_velocity;
            pos_direction = 1;
        } else if ((frame.id == make_can_id(DRIVE_POSITION)) && pos_ctrl) {
            target = int16_bytes_to_double(&frame.data[0]) * max_position;
            velocity = max_velocity / 4;
            pos_direction = position <= target ? 1 : -1;
            velocity *= pos_direction;
        }
    }

    void update_telemetry() {
        position += velocity * (pos_ctrl ? M_PI : 2 * M_PI) * send_rate.count() / 1000.0;
        if (pos_ctrl && pos_direction != 0) {
            if ((pos_direction > 0 && position > target) || (pos_direction < 0 && position < target)) {
                position = target;
                velocity = 0;
                pos_direction = 0;
            }
//                    cout << "position: " << position << endl;
//                    cout << "velocity: " << velocity << endl;
//                    cout << "pos_direction: " << pos_direction << endl;
        } else if (!pos_ctrl) {
            if (position > max_position) {
                position = position - max_position + min_position;
            } else if (position < 0) {
                position = position + max_position - min_position;
            }
        }
    }

    void send_telemetry() {
        Frame packet1;
        Frame packet2;
        Frame packet3;
        Frame packet4;


        packet1.id = make_can_id(PACKET_1);
        packet2.id = make_can_id(PACKET_2);
        packet3.id = make_can_id(PACKET_3);
        packet4.id = make_can_id(PACKET_4);

        packet1.data = {0, 0, 0, 0};
        packet2.data = {0, 0, 0, 0};

        int16_t velocity_data = convert_to_int16(velocity / max_velocity);
        int16_t position_data = convert_to_int16(position / max_position);

        packet3.data.push_back((uint8_t) (position_data >> 8));
        packet3.data.push_back((uint8_t) (position_data & 0xFF));
        packet3.data.push_back((uint8_t) (velocity_data >> 8));
        packet3.data.push_back((uint8_t) (velocity_data & 0xFF));

        packet4.data = {0, 0, 0, 0, 0, 0};


        bus->send(packet1);
        bus->send(packet2);
        bus->send(packet3);
        bus->send(packet4);

    }

    void spin() {
        bus->spin();
    }
};


class BLCMDSim : public rclcpp::Node{
private:
    vector<BLCMDSpoof> wheels;
    vector<BLCMDSpoof> pivots;

    rclcpp::TimerBase::SharedPtr spin_timer;
    rclcpp::TimerBase::SharedPtr send_timer;
    rclcpp::TimerBase::SharedPtr update_timer;


public:
    BLCMDSim(): Node("blcmd_sim") {
        wheels.push_back(BLCMDSpoof(false, 0, 20ms));
        wheels.push_back(BLCMDSpoof(false, 1, 20ms));
        wheels.push_back(BLCMDSpoof(false, 2, 10ms));
        wheels.push_back(BLCMDSpoof(false, 3, 10ms));

        pivots.push_back(BLCMDSpoof(true, 4, 20ms));
        pivots.push_back(BLCMDSpoof(true, 5, 20ms));
        pivots.push_back(BLCMDSpoof(true, 6, 20ms));
        pivots.push_back(BLCMDSpoof(true, 7, 10ms));

        spin_timer = this->create_wall_timer(10ms, std::bind(&BLCMDSim::spin, this));
        send_timer = this->create_wall_timer(20ms, std::bind(&BLCMDSim::send, this));
        update_timer = this->create_wall_timer(20ms, std::bind(&BLCMDSim::update, this));
    }

    void spin() {
        wheels[0].spin();
        wheels[1].spin();
        wheels[2].spin();
        wheels[3].spin();

        pivots[0].spin();
        pivots[1].spin();
        pivots[2].spin();
        pivots[3].spin();
    }

    void send() {
        wheels[0].send_telemetry();
        wheels[1].send_telemetry();
        wheels[2].send_telemetry();
        wheels[3].send_telemetry();

        pivots[0].send_telemetry();
        pivots[1].send_telemetry();
        pivots[2].send_telemetry();
        pivots[3].send_telemetry();
    }

    void update() {
        wheels[0].update_telemetry();
        wheels[1].update_telemetry();
        wheels[2].update_telemetry();
        wheels[3].update_telemetry();

        pivots[0].update_telemetry();
        pivots[1].update_telemetry();
        pivots[2].update_telemetry();
        pivots[3].update_telemetry();
    }

};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::spin(std::make_shared<BLCMDSim>());
    rclcpp::shutdown();
    return 0;
}