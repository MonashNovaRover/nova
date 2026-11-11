from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    return LaunchDescription([
        Node(
            package='usb_cam',
            executable='usb_cam_node_exe',
            name='usb_cam_node',
            output='screen',
            parameters=[{
                'video_device': "/dev/video4",
                'framerate': 120.0,
                'io_method': "mmap",
                'frame_id': "camera",
                'pixel_format': "yuyv2rgb",
                'av_device_format': "YUV422P",
                'image_width': 640,
                'image_height': 360,
                'camera_name': "test_camera",
                'camera_info_url': "file:///home/nova/nova/src/ros/rover/auto/auto_bringup/camera_info.yaml",
                'brightness': -1,
                'contrast': -1,
                'saturation': -1,
                'sharpness': -1,
                'gain': -1,
                'white_balance': 4000,
                'exposure': 100,
                'focus': -1
            }]
        )
    ])