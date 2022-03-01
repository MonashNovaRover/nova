#!/usr/bin/env python3

'''
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Monash Nova Rover Team

This class receives data from arm_coord_frames and
    plots the data in 3d-space.
It can take button-based user input to define task
    velocities for the IK system, and output the 
    task velocity as though it came from the joysticks.
Used to run tests on IK when the joysticks are unavailable.

~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: joystick_plotter
TOPICS:
  - /control/arm_coord_frames   [MultiDOFJointState]   [Subscribed]
  - /base/ljs_raw_ctrl          [Joy]                  [Published]
  - /base/rjs_raw_ctrl          [Joy]                  [Published]
SERVICES: None
ACTIONS:  None
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE: 	control
AUTHOR(S):	Thomas Cameron
CREATION:	25/01/2022
EDITED:		25/01/2022
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
'''
# TODO: Ensure that when speed is altered, cannot excede -1 or 1

# Import all ROS 2 packages
import rclpy
from rclpy.node import Node

#Import the required messages
from sensor_msgs.msg import MultiDOFJointState
from sensor_msgs.msg import Joy

# Import plotting tools
import matplotlib as mpl
import matplotlib.pyplot as plt
from matplotlib.widgets import Button
from mpl_toolkits.mplot3d import axes3d


# Import mathematics tools
import numpy as np

# Required for updating without bringing the window to the front
mpl.use("Qt5agg")
mpl.rc('axes.formatter', useoffset=False)

class Joystick_Plotter(Node):
  def __init__(self):
    super().__init("joystick_plotter")

    # Initialise subscriber
    self.arm_coord_sub = self.create_subscription(
      MultiDOFJointState, '/control/arm_coord_frames', self.draw, 10
    )
    
    # Initialise publishers
    self.left_joystick_pub = self.create_publisher(
      Joy, '/base/ljs_raw_ctrl', 1)
    self.right_joystick_pub = self.create_publisher(
      Joy, '/base/rjs_raw_ctrl', 1)

    # Set up variables

    # Variable for desired end effector velocity
    self.velocity = np.zeros(6) 

    # TODO: word the below line better
    # Speed change per button press (units of output to Joysticks)
    self.speed_joystick = 0.1


    # Set up plot figure
    self.fig = plt.figure()
    self.ax = self.fig.add_subplot(111, projection = '3d')
    self.fig.show()

    # Set up buttons
    # Linear velocities
    self.ax_xp = plt.axes([0.89, 0.51, 0.1, 0.075])
    self.button_xp = Button(self.ax_xp, 'X+')
    self.button_xp.on_clicked(self.xp)

    self.ax_xn = plt.axes([0.78, 0.51, 0.1, 0.075])
    self.button_xn = Button(self.ax_xn, 'X-')
    self.button_xn.on_clicked(self.xn)

    self.ax_yp = plt.axes([0.89, 0.41, 0.1, 0.075])
    self.button_yp = Button(self.ax_yp, 'Y+')
    self.button_yp.on_clicked(self.yp)

    self.ax_yn = plt.axes([0.78, 0.41, 0.1, 0.075])
    self.button_yn = Button(self.ax_yn, 'Y-')
    self.button_yn.on_clicked(self.yn)

    self.ax_zp = plt.axes([0.89, 0.31, 0.1, 0.075])
    self.button_zp = Button(self.ax_zp, 'Z+')
    self.button_zp.on_clicked(self.zp)

    self.ax_zn = plt.axes([0.78, 0.31, 0.1, 0.075])
    self.button_zn = Button(self.ax_zn, 'Z-')
    self.button_zn.on_clicked(self.zn)

    # Angular velocities
    self.angular_button_offset = 0.15
    self.ax_ap = plt.axes([0.89, 0.21, 0.1, 0.075])
    self.button_ap = Button(self.ax_ap, 'A+')
    self.button_ap.on_clicked(self.ap)

    self.ax_an = plt.axes([0.78, 0.21, 0.1, 0.075])
    self.button_an = Button(self.ax_an, 'A-')
    self.button_an.on_clicked(self.an)

    self.ax_bp = plt.axes([0.89, 0.11, 0.1, 0.075])
    self.button_bp = Button(self.ax_bp, 'B+')
    self.button_bp.on_clicked(self.bp)

    self.ax_bn = plt.axes([0.78, 0.11, 0.1, 0.075])
    self.button_bn = Button(self.ax_bn, 'B-')
    self.button_bn.on_clicked(self.bn)

    self.ax_cp = plt.axes([0.89, 0.01, 0.1, 0.075])
    self.button_cp = Button(self.ax_cp, 'C+')
    self.button_cp.on_clicked(self.cp)

    self.ax_cn = plt.axes([0.78, 0.01, 0.1, 0.075])
    self.button_cn = Button(self.ax_cn, 'C-')
    self.button_cn.on_clicked(self.cn)    
  
  # The callback function when the arm coord frames are received.
  # Plot the position of the arm using the position data.
  # Joints are marked with blue dots and connected by lines.
  def draw(self, msg):
    # Check plot is still open
    if not self.is_open():
      return
    
    # Clear the plot
    self.ax.cla()

    # Obtain list of transforms
    points = np.transpose(self.get_positions(msg))

    # Plot the arm
    # Plot points and lines for all joints and end effector
    X, Y, Z = points[:, :7]
    self.ax.plot(X, Y, Z, marker = 'o', markersize = 3, color = 'blue')
    # TODO: Plot a red dot at the end effector position

    # Plot pretty
    self.ax.set_xlabel("X")
    self.ax.set_ylabel("Y")
    self.ax.set_zlabel("Z")

    # TODO: Note that these positions are extracted from arm_coord_frames, check dimension/scale
    self.ax.set_xlim(-1000, 1000)
    self.ax.set_ylim(-1000, 1000)
    self.ax.set_zlim(-1000, 1000)

    # Draw without bringing window to front
    self.fig.canvas.draw_idle()
    self.fig.canvas.start_event_loop(0.001)


  # Publishes joystick topic when button is pressed on plotter.
  def publish(self):
    # New drive command messages
    ljs_msg = Joy() 
    rjs_msg = Joy()

    # Update both joystick messages to match current velocity array
    ljs_msg.ax_stick_x = self.limit(self.velocity[0])
    ljs_msg.ax_stick_y = self.limit(self.velocity[1])
    ljs_msg.ax_stick_twist = self.limit(self.velocity[2])

    rjs_msg.ax_stick_x = self.limit(self.velocity[3])
    rjs_msg.ax_stick_y = self.limit(self.velocity[4])
    rjs_msg.ax_stick_twist = self.limit(self.velocity[5])

    # Send messages
    self.left_joystick_pub.publish(ljs_msg)
    self.right_joystick_pub.publish(rjs_msg)

  def get_positions(self, msg):
    # Obtain list of transforms
    positions = msg.transforms.translation
    pos_array = np.array()
    for i in range(len(positions)):
      # Extract position data
      pos_array[i, 0] = positions[i].x
      pos_array[i, 1] = positions[i].y
      pos_array[i, 2] = positions[i].z

  def limit(self, value):
    # Returns 1.0 or -1.0 if passed value exceeds range. Otherwise, returns value
    if (abs(value) > 1): 
      return value/abs(value)
    else: return value

    # Return position data
    return pos_array



  # Callbacks for linear velocity buttons
  def xp(self, args):
      self.velocity[0] += self.speed_joystick
      self.publish()

  def xn(self, args):
      self.velocity[0] -= self.speed_joystick
      self.publish()

  def yp(self, args):
      self.velocity[1] += self.speed_joystick
      self.publish()

  def yn(self, args):
      self.velocity[1] -= self.speed_joystick
      self.publish()

  def zp(self, args):
      self.velocity[2] += self.speed_joystick
      self.publish()

  def zn(self, args):
      self.velocity[2] -= self.speed_joystick
      self.publish()

  # Callbacks for angular velocity buttons
  def ap(self, args):
      self.velocity[3] += self.speed_joystick
      self.publish()

  def an(self, args):
      self.velocity[3] -= self.speed_joystick
      self.publish()

  def bp(self, args):
      self.velocity[4] += self.speed_joystick
      self.publish()

  def bn(self, args):
      self.velocity[4] -= self.speed_joystick
      self.publish()

  def cp(self, args):
      self.velocity[5] += self.speed_joystick
      self.publish()

  def cn(self, args):
      self.velocity[5] -= self.speed_joystick
      self.publish()

  def is_open(self):
    return plt.fignum_exists(self.fig.number)

    

# Main function for setting up the ROS node
def main(args = None):
  rclpy.init(args = args)
  node = Joystick_Plotter()
  rclpy.spin(node)

  node.destroy_node()
  rclpy.shutdown()

# This code is called when 'python3' is used to run the script
  if __name__ == '__main__':
    main()
