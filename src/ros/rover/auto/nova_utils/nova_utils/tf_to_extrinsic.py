import rclpy
from rclpy.node import Node
import tf2_ros
import numpy as np
from scipy.spatial.transform import Rotation as R


class TFToExtrinsic(Node):
    def __init__(self):
        super().__init__('tf_to_extrinsic')

        self.declare_parameter('source_frame', 'oak_rgb_camera_optical_frame')
        self.declare_parameter('target_frame', 'livox_frame')

        self.source_frame = self.get_parameter('source_frame').value
        self.target_frame = self.get_parameter('target_frame').value

        self.tf_buffer = tf2_ros.Buffer()
        self.tf_listener = tf2_ros.TransformListener(self.tf_buffer, self)

        self.timer = self.create_timer(1.0, self.lookup_tf)

    def lookup_tf(self):
        try:
            tf = self.tf_buffer.lookup_transform(
                self.target_frame,
                self.source_frame,
                rclpy.time.Time()
            )

            t = tf.transform.translation
            q = tf.transform.rotation

            # Translation
            T = np.array([t.x, t.y, t.z])

            # Quaternion → rotation matrix
            quat = [q.x, q.y, q.z, q.w]
            rot = R.from_quat(quat).as_matrix()

            print("--- Extrinsic Calibration f---")
            print(f"{self.source_frame} to {self.target_frame}")
            print(f"extrinsic_T: [{T[0]:.6f}, {T[1]:.6f}, {T[2]:.6f}]")
            print("extrinsic_R: [")

            for row in rot:
                print(f"  {row[0]:.6f}, {row[1]:.6f}, {row[2]:.6f},")

            print("]")

            rclpy.shutdown()

        except Exception as e:
            self.get_logger().warn(f"TF lookup failed: {e}")


def main():
    rclpy.init()
    node = TFToExtrinsic()
    rclpy.spin(node)


if __name__ == '__main__':
    main()