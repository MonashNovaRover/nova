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


using namespace std;
using namespace org::jcan;

class BLCMDSim {
private:

    double velocity;
    double position;
    bool pos_ctrl;
    int id;
    int pos_direction;
    chrono::milliseconds send_rate;
    double target;
    org::jcan::Bus *send_bus;
    org::jcan::Bus *recv_bus;

public:
    BLCMDSim(bool pos_ctrl, int id, chrono::milliseconds send_rate): pos_ctrl(pos_ctrl), id(id), send_rate(send_rate) {
        target = 0;
        velocity = 0;
        position = 0;
        pos_direction = 0;
        shared_ptr<org::jcan::Bus> bus = org::jcan::open_bus("vcan0").into_raw();

        recv_bus->set_id_filter({make_can_id(DRIVE_VELOCITY), make_can_id(DRIVE_POSITION)});
    }

    void run() {
        auto last_time = chrono::high_resolution_clock::now();
//        auto now = chrono::high_resolution_clock::now();
//        unsigned int count = 0;
//        chrono::milliseconds total_time = send_rate;
        while (1) {
            auto frames = recv_bus->receive_nonblocking();

            for (auto frame : frames) {
                if (frame.id == make_can_id(DRIVE_VELOCITY)) {
                    velocity = int16_bytes_to_double(&frame.data[0])*max_velocity;
                    pos_direction = 1;
                } else if ((frame.id == make_can_id(DRIVE_POSITION)) && pos_ctrl) {
                    target = int16_bytes_to_double(&frame.data[0])*max_position;
                    velocity = max_velocity/4;
                    pos_direction = position <= target ? 1 : -1;
                    velocity *= pos_direction;
//                    cout << "target: " << target << endl;
//                    cout << "velocity: " << velocity << endl;
//                    cout << "pos_direction: " << pos_direction << endl;
                }
            }

            if ((chrono::high_resolution_clock::now() - last_time) >= send_rate) {
//                total_time += chrono::duration_cast<chrono::milliseconds>(chrono::high_resolution_clock::now() - last_time);
//                count++;
                position += velocity*(pos_ctrl ? M_PI : 2*M_PI) * send_rate.count() / 1000.0;
                if (pos_ctrl && pos_direction != 0) {
                    if ((pos_direction > 0 && position > target) || (pos_direction < 0 && position < target)){
                        position = target;
                        velocity = 0;
                        pos_direction = 0;
                    }
//                    cout << "position: " << position << endl;
//                    cout << "velocity: " << velocity << endl;
//                    cout << "pos_direction: " << pos_direction << endl;
                } else if (!pos_ctrl){
                    if (position > max_position){
                        position = position - max_position + min_position;
                    } else if (position < 0){
                        position = position + max_position - min_position;
                    }
                }

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


                send_bus->send(packet1);
                send_bus->send(packet2);
                send_bus->send(packet3);
                send_bus->send(packet4);
                last_time = chrono::high_resolution_clock::now();
            }
        }

//        cout << "Average send rate: " << count / (total_time.count() / 1000.0) << "Hz" << endl;
//
//        cout << "Average send time: " << total_time.count() / count << "ms" << endl;
//
//        cout << "Send rate: " << send_rate.count() << "ms" << endl;
//
//        cout << "Velocity: " << velocity << endl;
//
//        cout << "Position: " << position << endl;
    }

    int16_t convert_to_int16 (const double value) {
        // Convert the value to an integer
        return (int16_t)(value * 32767.0f);
    }

    int16_t from_bytes(uint8_t *bytes) {
        return (bytes[0] << 8) | bytes[1];
    }

    double int16_bytes_to_double (uint8_t* bytes)
    {
        // Scale the value to a double
        return from_bytes(bytes)/32767.0;
    }

    double uint16_bytes_to_double (uint8_t* bytes)
    {
        // Scale the value to a double
        return from_bytes(bytes)/65535.0;
    }

    uint16_t make_can_id(BLCMDSendCommand command)
    {
        return SEND << 8 | id << 4 | command;
    }

    uint16_t make_can_id(BLCMDReceiveCommand command)
    {
        return RECEIVE << 8 | id << 4 | command;
    }

    uint16_t make_can_id(TelemetryPacket packet)
    {
        return RECEIVE << 8 | id << 4 | packet;
    }

};

void run_sim(BLCMDSim *blcmd)
{
    blcmd->run();
}


int main(int argc, char **argv)
{
    // Create the BLCMDs
    BLCMDSim front_left_wheel(false, 1, (1000/20)*1ms);
    BLCMDSim front_left_pivot(true, 5, (1000/20)*1ms);
    BLCMDSim back_left_wheel(false, 2, (1000/20)*1ms);
    BLCMDSim back_left_pivot(true, 6, (1000/20)*1ms);

    BLCMDSim back_right_wheel(false, 3, (1000/20)*1ms);
    BLCMDSim back_right_pivot(true, 7, (1000/20)*1ms);

    BLCMDSim front_right_wheel(false, 4, (1000/20)*1ms);
    BLCMDSim front_right_pivot(true, 8, (1000/20)*1ms);



    // Create the threads
    thread front_left_wheel_thread(&run_sim, &front_left_wheel);
    thread front_left_pivot_thread(&BLCMDSim::run, front_left_pivot);
    thread front_right_wheel_thread(&BLCMDSim::run, front_right_wheel);
    thread front_right_pivot_thread(&BLCMDSim::run, front_right_pivot);
    thread back_left_wheel_thread(&BLCMDSim::run, back_left_wheel);
    thread back_left_pivot_thread(&BLCMDSim::run, back_left_pivot);
    thread back_right_wheel_thread(&BLCMDSim::run, back_right_wheel);
    thread back_right_pivot_thread(&BLCMDSim::run, back_right_pivot);
    // Join the threads
    front_left_wheel_thread.join();
    front_left_pivot_thread.join();
    front_right_wheel_thread.join();
    front_right_pivot_thread.join();
    back_left_wheel_thread.join();
    back_left_pivot_thread.join();
    back_right_wheel_thread.join();
    back_right_pivot_thread.join();


    return 0;

}
