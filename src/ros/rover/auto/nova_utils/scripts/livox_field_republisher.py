#!/usr/bin/env python3

from struct import pack_into, unpack_from
import math

import rclpy
from rclpy.node import Node
from sensor_msgs.msg import PointCloud2, PointField


class LivoxFieldRepublisher(Node):
    """Republish PointCloud2 into a Livox Mid-360 field layout.

    Output layout per point (26 bytes):
      x: float32 (offset 0)
      y: float32 (offset 4)
      z: float32 (offset 8)
      intensity: float32 (offset 12)
      tag: uint8 (offset 16)
      line: uint8 (offset 17)
      timestamp: float64 (offset 18)
    """

    def __init__(self) -> None:
        super().__init__("livox_field_republisher")

        self.declare_parameter("input_topic", "/livox/lidar_sim")
        self.declare_parameter("output_topic", "/livox/lidar")

        input_topic = self.get_parameter("input_topic").get_parameter_value().string_value
        output_topic = self.get_parameter("output_topic").get_parameter_value().string_value
        self.default_tag = 0x10
        self.scan_period_sec = 0.1
        self.drop_invalid_points = True

        self.pub = self.create_publisher(PointCloud2, output_topic, 10)
        self.sub = self.create_subscription(PointCloud2, input_topic, self.cb, 10)

        self.get_logger().info(
            f"Republishing PointCloud2 from {input_topic} to {output_topic} with Livox-like fields"
        )

    @staticmethod
    def _field_map(msg: PointCloud2) -> dict[str, PointField]:
        return {f.name: f for f in msg.fields}

    @staticmethod
    def _read_float32(data: bytes, base: int, field: PointField, endian: str) -> float:
        return unpack_from(f"{endian}f", data, base + field.offset)[0]

    @staticmethod
    def _read_uint8(data: bytes, base: int, field: PointField) -> int:
        return data[base + field.offset]

    @staticmethod
    def _read_float64(data: bytes, base: int, field: PointField, endian: str) -> float:
        return unpack_from(f"{endian}d", data, base + field.offset)[0]

    @staticmethod
    def _read_uint32(data: bytes, base: int, field: PointField, endian: str) -> int:
        return unpack_from(f"{endian}I", data, base + field.offset)[0]

    @staticmethod
    def _to_epoch_ns(stamp) -> float:
        return float(stamp.sec) * 1e9 + float(stamp.nanosec)

    def cb(self, msg: PointCloud2) -> None:
        fmap = self._field_map(msg)
        required = ["x", "y", "z"]
        missing = [name for name in required if name not in fmap]
        if missing:
            self.get_logger().error(f"Missing required fields in input cloud: {missing}")
            return

        endian = ">" if msg.is_bigendian else "<"
        total_points = msg.width * msg.height
        in_step = msg.point_step

        out = PointCloud2()
        out.header = msg.header
        out.height = 1
        out.width = 0
        out.is_bigendian = msg.is_bigendian
        out.is_dense = False

        out.fields = [
            PointField(name="x", offset=0, datatype=PointField.FLOAT32, count=1),
            PointField(name="y", offset=4, datatype=PointField.FLOAT32, count=1),
            PointField(name="z", offset=8, datatype=PointField.FLOAT32, count=1),
            PointField(name="intensity", offset=12, datatype=PointField.FLOAT32, count=1),
            PointField(name="tag", offset=16, datatype=PointField.UINT8, count=1),
            PointField(name="line", offset=17, datatype=PointField.UINT8, count=1),
            PointField(name="timestamp", offset=18, datatype=PointField.FLOAT64, count=1),
        ]
        out.point_step = 26
        out.row_step = out.point_step * out.width

        # Reserve max possible size and shrink once valid point count is known.
        out_data = bytearray(total_points * out.point_step)

        x_f = fmap["x"]
        y_f = fmap["y"]
        z_f = fmap["z"]
        intensity_f = fmap.get("intensity")
        tag_f = fmap.get("tag")
        line_f = fmap.get("line") or fmap.get("ring")
        timestamp_f = fmap.get("timestamp")
        t_f = fmap.get("t")

        header_ns = self._to_epoch_ns(msg.header.stamp)
        dt_ns = 0.0
        if total_points > 1 and self.scan_period_sec > 0.0:
            dt_ns = (self.scan_period_sec * 1e9) / float(total_points - 1)

        has_invalid = False
        written_points = 0

        for i in range(total_points):
            in_base = i * in_step

            x = self._read_float32(msg.data, in_base, x_f, endian)
            y = self._read_float32(msg.data, in_base, y_f, endian)
            z = self._read_float32(msg.data, in_base, z_f, endian)
            valid_xyz = math.isfinite(x) and math.isfinite(y) and math.isfinite(z)
            if not valid_xyz:
                has_invalid = True
                if self.drop_invalid_points:
                    continue

            out_base = written_points * out.point_step
            intensity = (
                self._read_float32(msg.data, in_base, intensity_f, endian) if intensity_f is not None else 0.0
            )
            tag = self._read_uint8(msg.data, in_base, tag_f) if tag_f is not None else self.default_tag
            if msg.height > 1 and msg.width > 0:
                line = i // msg.width
            else:
                line = 0

            if timestamp_f is not None:
                timestamp = self._read_float64(msg.data, in_base, timestamp_f, endian)
            elif t_f is not None:
                timestamp = float(self._read_uint32(msg.data, in_base, t_f, endian))
            else:
                timestamp = header_ns + dt_ns * float(i)

            pack_into(f"{endian}f", out_data, out_base + 0, x)
            pack_into(f"{endian}f", out_data, out_base + 4, y)
            pack_into(f"{endian}f", out_data, out_base + 8, z)
            pack_into(f"{endian}f", out_data, out_base + 12, intensity)
            out_data[out_base + 16] = tag & 0xFF
            out_data[out_base + 17] = line & 0xFF
            pack_into(f"{endian}d", out_data, out_base + 18, timestamp)

            written_points += 1

        out.height = 1
        out.width = written_points
        out.row_step = out.point_step * out.width
        out.data = bytes(out_data[: written_points * out.point_step])

        out.is_dense = self.drop_invalid_points or (not has_invalid)
        self.pub.publish(out)


def main(args=None) -> None:
    rclpy.init(args=args)
    node = LivoxFieldRepublisher()
    try:
        rclpy.spin(node)
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
