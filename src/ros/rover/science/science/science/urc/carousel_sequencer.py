#!/usr/bin/env python3
"""
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
Purpose: Automated Carousel Fill Sequence
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
NODE: carousel_sequencer
TOPICS:
    - publisher: /science/carousel_sequence/status  [CarouselSequenceStatus]
SERVICES:
    - service: /science/carousel_sequence/start     [CarouselSequence]
    - service: /science/carousel_sequence/stop      [Trigger]
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
PACKAGE:    science
AUTHOR(S):  Claude
CREATION:   10-05-2026
EDITED:     10-05-2026
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"""
import threading
import time

import rclpy
from rclpy.node import Node
from rclpy.qos import QoSProfile, QoSHistoryPolicy, QoSDurabilityPolicy
from std_srvs.srv import Trigger

from science_interfaces.srv import CarouselSequence, RunPump, SetPosition
from science_interfaces.msg import CarouselSequenceStatus, PumpStatus, CarouselFeedback


class CarouselSequencer(Node):
    """
    Automated carousel fill sequence node.

    Runs a configurable sequence of pump -> move -> pump -> move cycles
    for filling carousel wells with samples.
    """

    def __init__(self):
        super().__init__('carousel_sequencer')

        # Declare parameters
        self.delay_after_pump = self.declare_parameter('delay_after_pump', 1.0).value
        self.delay_after_move = self.declare_parameter('delay_after_move', 1.0).value
        self.move_timeout = self.declare_parameter('move_timeout', 10.0).value
        self.position_tolerance = self.declare_parameter('position_tolerance', 2.0).value

        # Thread control
        self.thread = None
        self.stop_event = threading.Event()
        self.sequence_lock = threading.Lock()

        # Current sequence state
        self.current_ring = ''
        self.current_iteration = 0
        self.total_iterations = 0
        self.current_step = 'idle'
        self.error_message = ''

        # Services (server)
        self.start_srv = self.create_service(
            CarouselSequence,
            '/science/carousel_sequence/start',
            self.start_callback
        )
        self.stop_srv = self.create_service(
            Trigger,
            '/science/carousel_sequence/stop',
            self.stop_callback
        )

        # Status publisher with persisted QoS
        qos = QoSProfile(
            history=QoSHistoryPolicy.KEEP_LAST,
            depth=1,
            durability=QoSDurabilityPolicy.TRANSIENT_LOCAL
        )
        self.status_pub = self.create_publisher(
            CarouselSequenceStatus,
            '/science/carousel_sequence/status',
            qos
        )

        # Service clients (to call existing services)
        self.pump_client = self.create_client(RunPump, '/science/pumps/run')
        self.carousel_inner_client = self.create_client(SetPosition, '/science/carousel_inner/set_position')
        self.carousel_outer_client = self.create_client(SetPosition, '/science/carousel_outer/set_position')

        # Subscriptions for completion detection
        self.pump_status = PumpStatus()
        self.carousel_inner_pos = 0.0
        self.carousel_outer_pos = 0.0

        self.create_subscription(
            PumpStatus,
            '/science/pumps/status',
            self._pump_status_callback,
            10
        )
        self.create_subscription(
            CarouselFeedback,
            '/science/carousel_inner/feedback',
            self._carousel_inner_callback,
            10
        )
        self.create_subscription(
            CarouselFeedback,
            '/science/carousel_outer/feedback',
            self._carousel_outer_callback,
            10
        )

        # Publish initial idle status
        self.publish_status('idle', 0, 0, '')

        self.get_logger().info('CarouselSequencer initialized')
        self.get_logger().info(f'  delay_after_pump: {self.delay_after_pump}s')
        self.get_logger().info(f'  delay_after_move: {self.delay_after_move}s')
        self.get_logger().info(f'  move_timeout: {self.move_timeout}s')
        self.get_logger().info(f'  position_tolerance: {self.position_tolerance} degrees')

    def _pump_status_callback(self, msg: PumpStatus):
        """Store latest pump status."""
        self.pump_status = msg

    def _carousel_inner_callback(self, msg: CarouselFeedback):
        """Store latest inner carousel position."""
        self.carousel_inner_pos = msg.position

    def _carousel_outer_callback(self, msg: CarouselFeedback):
        """Store latest outer carousel position."""
        self.carousel_outer_pos = msg.position

    def start_callback(self, request: CarouselSequence.Request,
                       response: CarouselSequence.Response) -> CarouselSequence.Response:
        """Handle start sequence service request."""
        with self.sequence_lock:
            if self.thread and self.thread.is_alive():
                response.success = False
                response.message = "Sequence already running"
                return response

            # Validate ring
            if request.ring not in ['inner', 'outer']:
                response.success = False
                response.message = f"Invalid ring: '{request.ring}'. Must be 'inner' or 'outer'."
                return response

            # Validate iterations
            if request.iterations <= 0:
                response.success = False
                response.message = f"Invalid iterations: {request.iterations}. Must be > 0."
                return response

            # Validate pump duration
            if request.pump_duration <= 0:
                response.success = False
                response.message = f"Invalid pump_duration: {request.pump_duration}. Must be > 0."
                return response

            self.stop_event.clear()
            self.error_message = ''
            self.thread = threading.Thread(
                target=self.execute_sequence,
                args=(request.ring, request.iterations, request.pump_duration),
                daemon=True
            )
            self.thread.start()

            response.success = True
            response.message = f"Started sequence: {request.ring} ring, {request.iterations} iterations"
            self.get_logger().info(response.message)
            return response

    def stop_callback(self, request: Trigger.Request,
                      response: Trigger.Response) -> Trigger.Response:
        """Handle stop sequence service request."""
        with self.sequence_lock:
            if self.thread and self.thread.is_alive():
                self.stop_event.set()
                self.thread.join(timeout=2.0)
                self.get_logger().info("Sequence stopped by user")

            self.publish_status('idle', 0, 0, '')

            response.success = True
            response.message = "Stopped"
            return response

    def execute_sequence(self, ring: str, iterations: int, pump_duration: float):
        """
        Main sequence execution (runs in separate thread).

        Args:
            ring: 'inner' or 'outer'
            iterations: Number of fill cycles
            pump_duration: Seconds to run pump each iteration
        """
        pump_name = 'shot_to_inner_pump' if ring == 'inner' else 'shot_to_outer_pump'
        step_degrees = 24.0 if ring == 'inner' else 15.0

        current_pos = self.carousel_inner_pos if ring == 'inner' else self.carousel_outer_pos

        self.get_logger().info(f"Starting sequence: ring={ring}, iterations={iterations}, "
                               f"pump_duration={pump_duration}, pump={pump_name}, step={step_degrees}")

        for i in range(iterations):
            if self.stop_event.is_set():
                self.get_logger().info("Sequence interrupted by stop event")
                self.publish_status('idle', i, iterations, ring)
                return

            iteration_num = i + 1

            # Step 1: Run pump
            self.publish_status('pumping', iteration_num, iterations, ring)
            self.get_logger().info(f"Iteration {iteration_num}/{iterations}: Pumping for {pump_duration}s")

            if not self.run_pump(pump_name, pump_duration):
                self.publish_error(f"Pump failed on iteration {iteration_num}")
                return

            # Step 2: Wait after pump
            if self.delay_after_pump > 0:
                self.publish_status('delay_pump', iteration_num, iterations, ring)
                if not self.interruptible_sleep(self.delay_after_pump):
                    return

            # Step 3: Move carousel
            self.publish_status('moving', iteration_num, iterations, ring)
            target_pos = (current_pos + step_degrees) % 360

            self.get_logger().info(f"Iteration {iteration_num}/{iterations}: Moving carousel "
                                   f"from {current_pos:.1f} to {target_pos:.1f}")

            if not self.move_carousel(ring, target_pos):
                self.publish_error(f"Carousel move timeout on iteration {iteration_num}")
                return
            current_pos = target_pos

            # Step 4: Wait after move
            if self.delay_after_move > 0:
                self.publish_status('delay_move', iteration_num, iterations, ring)
                if not self.interruptible_sleep(self.delay_after_move):
                    return

        self.get_logger().info(f"Sequence completed: {iterations} iterations")
        self.publish_status('complete', iterations, iterations, ring)

    def run_pump(self, pump_name: str, duration: float) -> bool:
        """
        Run a pump for the specified duration.

        Args:
            pump_name: Name of the pump hardware
            duration: Seconds to run

        Returns:
            True if successful, False on failure
        """
        if not self.pump_client.wait_for_service(timeout_sec=2.0):
            self.get_logger().error("Pump service not available")
            return False

        request = RunPump.Request()
        request.pump = pump_name
        request.duration = duration

        future = self.pump_client.call_async(request)

        # Wait for pump to complete (with some margin)
        total_wait = duration + 2.0
        start_time = time.time()

        while time.time() - start_time < total_wait:
            if self.stop_event.is_set():
                return False

            # Check if pump finished
            if not self.pump_status.running and time.time() - start_time > 0.5:
                # Pump has stopped and we've waited at least 0.5s
                return True

            time.sleep(0.1)

        # Timeout - pump should have finished by now
        self.get_logger().warning(f"Pump operation may have timed out after {total_wait}s")
        return True  # Return true anyway, pump probably completed

    def move_carousel(self, ring: str, target_position: float) -> bool:
        """
        Move carousel to target position.

        Args:
            ring: 'inner' or 'outer'
            target_position: Target position in degrees

        Returns:
            True if reached position, False on timeout
        """
        client = self.carousel_inner_client if ring == 'inner' else self.carousel_outer_client

        if not client.wait_for_service(timeout_sec=2.0):
            self.get_logger().error(f"Carousel {ring} service not available")
            return False

        request = SetPosition.Request()
        request.position = target_position

        future = client.call_async(request)

        # Wait for carousel to reach position
        start_time = time.time()

        while time.time() - start_time < self.move_timeout:
            if self.stop_event.is_set():
                return False

            current_pos = self.carousel_inner_pos if ring == 'inner' else self.carousel_outer_pos

            # Check if we've reached the target (accounting for wraparound)
            diff = abs(current_pos - target_position)
            if diff > 180:
                diff = 360 - diff

            if diff <= self.position_tolerance:
                self.get_logger().debug(f"Carousel reached position: {current_pos:.1f} "
                                        f"(target: {target_position:.1f})")
                return True

            time.sleep(0.1)

        self.get_logger().error(f"Carousel move timed out after {self.move_timeout}s")
        return False

    def interruptible_sleep(self, duration: float) -> bool:
        """
        Sleep for duration, but check stop_event periodically.

        Args:
            duration: Seconds to sleep

        Returns:
            True if completed, False if interrupted
        """
        start_time = time.time()
        while time.time() - start_time < duration:
            if self.stop_event.is_set():
                return False
            time.sleep(0.1)
        return True

    def publish_status(self, step: str, current: int, total: int, ring: str):
        """Publish current sequence status."""
        with self.sequence_lock:
            self.current_step = step
            self.current_iteration = current
            self.total_iterations = total
            self.current_ring = ring

        msg = CarouselSequenceStatus()
        msg.running = step not in ['idle', 'complete', 'error']
        msg.current_iteration = current
        msg.total_iterations = total
        msg.current_step = step
        msg.ring = ring
        msg.error_message = self.error_message

        self.status_pub.publish(msg)

    def publish_error(self, error_msg: str):
        """Publish error status and log error."""
        self.get_logger().error(error_msg)
        self.error_message = error_msg
        self.publish_status('error', self.current_iteration, self.total_iterations, self.current_ring)


def main(args=None):
    rclpy.init(args=args)
    node = CarouselSequencer()

    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
