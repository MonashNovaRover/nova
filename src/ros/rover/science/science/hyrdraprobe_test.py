from pymodbus.client import ModbusSerialClient
import csv
import datetime
from pathlib import Path
import time

# client = ModbusSerialClient('COM3', baudrate=9600, bytesize=8, parity='N',stopbits=1, retries=2, broadcast_enable=True)
# client.connect()


# # client.write_register(0x0020, 0, 18)                            # set register 0x0020 to 0
# # print(client.read_holding_registers(0x0020, 1, 18).registers)   # confirm change

# regs = client.read_holding_registers(0x0200, count=1, slave=18)
# # time.sleep(10)
# print(regs)
# print(regs.registers)

# client.write_register(0x0020, 1, 18)
# print(client.read_holding_registers(0x0020, 1, 18).registers)

# regs = client.read_holding_registers(0, count=3, slave=18)

# print(regs)
# print(regs.registers)

# client.close()


"""
    - change soil type (use dict)
    - get temp ec moisture (all together vs specific)
    - get baud rate, address...
    - 

"""

def config_device(port, baudrate=9600, bytesize=8, parity='N', stopbits=1, retries=1, broadcast_enable=True):
    client = ModbusSerialClient(port, baudrate=baudrate, bytesize=bytesize, parity=parity, stopbits=stopbits, retries=retries, broadcast_enable=broadcast_enable)

    assert client.connect()

    return client

def read_moisture(client, slave=1):
    regs = client.read_holding_registers(0x0001, count=1, slave=1)
    try:
        val = regs.registers[0]/100
    except AttributeError:
        val = read_moisture(client, slave)
    
    return val

def read_temp(client, slave=1):
    regs = client.read_holding_registers(0x0000, count=1, slave=1)
    try:
        val = regs.registers[0]/100
    except AttributeError:
        val = read_temp(client, slave)
    
    return val

def read_ec(client, slave=1):
    regs = client.read_holding_registers(0x0002, count=1, slave=1)
    try:
        val = regs.registers[0]
    except AttributeError:
        val = read_ec(client, slave)
    
    return val

def read_epsilon(client, slave=1):
    regs = client.read_holding_registers(0x0005, count=1, slave=1)
    try:
        val = regs.registers[0]/100
    except AttributeError:
        val = read_temp(client, slave)
    
    return val

def read_all(client, slave=1):
    ec = read_ec(client, slave)
    time.sleep(0.1)
    moisture = read_moisture(client, slave)
    time.sleep(0.1)
    temp = read_temp(client, slave)
    time.sleep(0.1)
    eps = read_epsilon(client, slave)

    return temp, moisture, ec, eps


def save_data(data, fpath):
    date ='{date:%Y-%m-%d_%H.%M.%S}'.format( date=datetime.datetime.now() )
    filename = Path(fpath + 'data_' + date + '.csv')

    with open(filename, 'w') as fp:
        w = csv.writer(fp, delimiter=',')

        w.writerows(data)


def set_soil_type(soil, client, slave=1):
    match soil.lower():
        case 'mineral':
            soil = 0
        case 'sand':
            soil = 1
        case 'clay':
            soil = 2
        case 'organic':
            soil = 3
        case _:
            print(f'soil type {soil} is invalid')
            print(f"please select one of {['sand', 'mineral', 'clay', 'organic']}")

    client.write_register(0x0020, soil, count=1, slave=slave)

    assert client.read_holding_registers(0x0020, count=1, slave=slave).registers[0] == soil

def main():
    port = input('please input the port the device is connected to (i.e. COM3): ')
    if port == "":
        port = 'COM3'
    client = config_device(port)

    soil = input('what kind of soil are you testing with? (one of sand, mineral, clay, organic)\n')
    if soil == "":
        soil = 'sand'
    set_soil_type(soil, client)

    cont_flag = input('do you want continuous measurements? [y/n] ')
    assert cont_flag.lower() in ['y', 'n']

    if cont_flag == 'y':
        save_flag = input('do you want to save the data to csv? [y/n] ')
        assert save_flag.lower() in ['y', 'n']  

        data = []  

    print('What measurements do you want?')
    print('1. Temp\n2.Moisture\n3.EC\n4.Epsilon\n5.All 4')
    things = int(input(""))
    assert things > 0 and things < 6

    if things == 1:
        func = read_temp
    elif things == 2:
        func = read_moisture
    elif things == 3:
        func = read_ec
    elif things == 4:
        func = read_epsilon
    else:
        func = read_all

    if cont_flag.lower() == 'y':
        try:
            while True:
                vals = func(client)
                if save_flag.lower() == 'y':
                    data.append(vals)
                print(vals)
        except KeyboardInterrupt:
            pass
    else:
        vals = func(client)
        print(vals)

    if save_flag.lower() == 'y':
        fpath = input('where would you like to save your data? (enter filepath) ')
        if fpath == "":
            fpath = 'C:/Users/shelb/Downloads/'
        save_data(data, fpath)

    client.close()


if __name__ == "__main__":
    main()