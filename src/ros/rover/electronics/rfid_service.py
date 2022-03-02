# TODO: Add timeouts to reading
# TODO: more sophisticated write error handling
from core.srv import RFIDCommand, RFIDCommandResponse

import rclpy
from rclpy.node import Node

from serial import Serial

class RFIDService(Node):

    def __init__(self):
        super().__init__('rfid_service')
        self.srv = self.create_service(RFIDCommand, '/electronics/rfid_service', self.handle_rfid_request)
        self.ser = Serial(baudrate = 115200, port = '/dev/ttyUSB0') # TODO: Check port
        self.EOM_byte = b'\x00' # null char for End Of Message

    def handle_rfid_request(self, request: RFIDCommand, response: RFIDCommandResponse):
        cmd = request.command.lower()

        if cmd == 'read':
            response.response = self.read_data()
        elif cmd == 'write':
            write_data(bytearray(request.data, encoding='ascii'))
            response.response = self.read_data()
        else:
            # catch invalid commands
            msg = f'Service request refused: Invalid command: {cmd}'
            self.get_logger().error(msg)
            response.response = msg
            return response
        
        self.get_logger().debug('Response received from arduino: {response.response}')
        return response

    def read_data(self) -> str:
        '''
        Data is null terminated so simply read until null string sent
        '''
        data = bytearray()
        # read until null char
        while (val := self.ser.read(1)) != self.EOM_byte:
            data.append(val)

        # return as string
        return data.decode('ascii')

    def write_data(self, data: bytearray):
        '''
        Write ascii encoded bytearray data to controlling arduino.
        '''
        data.append(self.EOM_byte) # indicate end of message
        if (n := self.ser.write(data)) != len(data):
            self.get_logger().error(f'Only wrote {n} of {len(data)} bytes expected')
            # resend null termination to ensure micro doesn't continue waiting
            self.ser.write(self.EOM_byte)

    def destroy_node(self):
        self.get_logger().info('Closing serial connection')
        self.ser.close()
        return super().destroy_node()

def main(args=None):
    rclpy.init(args=args)

    rfid_service = RFIDService()

    rclpy.spin(rfid_service)

    rfid_service.destroy_node()

    rclpy.shutdown()


if __name__ == '__main__':
    main()
