import rosbag2_py
from sys import argv
from rclpy.serialization import serialize_message, deserialize_message
from rosidl_runtime_py.utilities import get_message
from geometry_msgs.msg import TwistStamped

def negate_vector3(vector3) -> None:
    vector3.x *= -1
    vector3.y *= -1
    vector3.z *= -1


if __name__ == "__main__":
    reader = rosbag2_py.SequentialReader()
    reader.open_uri(argv[1])
    reader.set_read_order(rosbag2_py.ReadOrder(sort_by=rosbag2_py.ReadOrderSortBy(0), reverse=True))
    metadata = reader.get_metadata()
    starting_time = metadata.starting_time.nanoseconds
    ending_time = metadata.starting_time.nanoseconds + metadata.duration.nanoseconds
    reader.seek(ending_time + 10)

    writer = rosbag2_py.SequentialWriter()
    writer.open(rosbag2_py.StorageOptions(uri=argv[2]),
                rosbag2_py.ConverterOptions('mcap','mcap'))

    new_metadata = rosbag2_py.TopicMetadata(
        id=0,
        name='/cmd_vel',
        type='geometry_msgs/msg/TwistStamped',
        serialization_format='cdr',
    )
    writer.create_topic(new_metadata)

    next_msg_time = 0

    while reader.has_next():
        (topic_name, data, msg_time) = reader.read_next()
        msg = deserialize_message(data, get_message('geometry_msgs/msg/TwistStamped'))

        negate_vector3(msg.twist.linear)
        # negate_vector3(msg.twist.angular)

        writer.write(topic_name, serialize_message(msg), next_msg_time)

        next_msg_time = ending_time - msg_time

    # TODO: DOES THIS TWISTSTAMPED (AND ALL OTHER TWISTSTAMPED) NEED TO BE UPDATED TO CORRECT VALUES????
    end_msg = TwistStamped()
    end_msg.twist.angular.x = 0
    end_msg.twist.angular.y = 0
    end_msg.twist.angular.z = 0
    end_msg.twist.linear.x = 0
    end_msg.twist.linear.y = 0
    end_msg.twist.linear.z = 0
    writer.write('/cmd_vel', serialize_message(end_msg), next_msg_time)

    del writer