import argparse
import os
import serial
import prettytable

CMD_DUMP = b'\x01'
CMD_ERASE = b'\x02'
CMD_WRITE_BYTE = b'\x03'
CMD_WRITE_FILE = b'\x04'
CMD_READ_BYTE = b'\x05'

eeprom_size = 0

def ensure_ack(ser: serial.Serial):
    ack = ser.readline().decode('ascii').strip()
    if ack != 'ACK':
        raise Exception('reply not ack')


def dump_eeprom(ser: serial.Serial):
    
    ser.write(CMD_DUMP + b'\x00\x00\x00')
    ensure_ack(ser)

    eeprom_content = ser.read(eeprom_size)
    ensure_ack(ser)

    return eeprom_content

def make_connection(port):
    ser = serial.Serial(port, baudrate=57600)

    eeprom_size = int(ser.readline().decode('ascii').strip())
    ensure_ack(ser)

    return ser


def parse_argument():
    parser = argparse.ArgumentParser(usage='An EEPROM programmer', prog='eeprom')
    parser.add_argument('-d', '--dump', help='dump current EEPROM content', action='store_true')
    parser.add_argument('-p', '--port', type=str, metavar='PORT', help='Serial port', required=True)
    parser.add_argument('-e', '--erase', type=str, metavar='BYTE',
                        help='Fill byte to EEPROM')
    parser.add_argument('-wf', '--write-file', type=str, metavar='PATH', help='Write a file to EEPROM')
    parser.add_argument('-wb', '--write-byte', nargs=2, type=str, metavar=('ADDRESS', 'BYTE'),
                        help='Write a single byte to EEPROM')
    parser.add_argument('-rb', '--read-byte', type=str, metavar='ADDRESS', help='Read a single byte from EEPROM')

    arg = parser.parse_args()
    return arg


def main():
    arguments = parse_argument()

    print('Connecting to device ... ')
    programmer = make_connection(arguments.port)

    if arguments.dump:
        dump = dump_eeprom(programmer, eeprom_size)
        
        table = prettytable.PrettyTable()
        table.field_names = ['ADDRESS'] + ['%02X' % i for i in range (0, 16)]
        for base in range(0, eeprom_size, 16):
            table.add_row(['%02X' % base] + ['02X' % d for d in dump[base + 0:base + 16]])
        print(table)

    elif arguments.erase is not None:
        print('Erasing EEPROM ... ')
        erase_byte = int(arguments.erase, 0).to_bytes(1, 'little')

        programmer.write(CMD_ERASE + erase_byte + b'\x00\x00')
        ensure_ack(programmer)

        ensure_ack(programmer)

    elif len(arguments.write_byte) == 2:
        address = int(arguments.write_byte[0], 0).to_bytes(2, 'little')
        byte = int(arguments.write_byte[1], 0).to_bytes(1, 'little')

        programmer.write(CMD_WRITE_BYTE + address + byte)
        ensure_ack(programmer)

        ensure_ack(programmer)

    elif arguments.write_file is not None:
        file_size = os.path.getsize(arguments.write_file)
        if file_size > eeprom_size:
            raise Exception('File can not bigger than %d' % eeprom_size)

        file_size = file_size.to_bytes(2, 'little')

        programmer.write(CMD_WRITE_FILE + file_size + b'\x00')
        ensure_ack()

        file = open(arguments.write_file, 'rb')
        programmer.write(file.read())
        ensure_ack()

        ensure_ack(programmer)

    elif len(arguments.read_byte) == 2:
        address = int(arguments.read_byte, 0).to_bytes(2, 'little')
        programmer.write(CMD_READ_BYTE + address + b'\x00')
        ensure_ack(programmer)

        content = programmer.read(1)
        ensure_ack(programmer)

        print('%02X' % content[0])


    else:
        pass

    programmer.close()

    return 0


if __name__ == '__main__':
    main()
