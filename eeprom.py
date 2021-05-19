import argparse
import os
import serial


def ensure_ack(ser: serial.Serial):
    ack = ser.readline().decode('ascii').strip()
    if ack != 'ACK':
        raise Exception('reply not ack')


def dump_eeprom(ser: serial.Serial, sz):
    ser.write(b'\x01\x00\x00\x00')
    eeprom_content = ser.read(sz)
    ensure_ack(ser)
    return eeprom_content


def make_connection(port):
    ser = serial.Serial(port, baudrate=57600)
    ensure_ack(ser)
    sz = int(ser.readline().decode('ascii').strip())
    return ser, sz


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
    print(arguments)
    print('Connecting to device ... ')
    programmer, size = make_connection(arguments.port)

    if arguments.dump:
        dump = dump_eeprom(programmer, size)
        print('Total %d' % len(dump))

        output = '        '
        for i in range(0, 16):
            output += '|  %02X  ' % i
        print(output)

        for base in range(0, len(dump), 16):
            output = '%04X    ' % base
            for offset in range(0, 16, 1):
                output += '|  %02X  ' % dump[base + offset]
            print(output)

    elif arguments.erase is not None:
        print('Erasing EEPROM ... ')
        erase_byte = int(arguments.erase, 0).to_bytes(1, 'little')
        programmer.write(b'\x02' + erase_byte + b'\x00\x00')
        ensure_ack(programmer)
        programmer.close()

        print('Validating EEPROM ...')
        programmer, size = make_connection(arguments.port)
        dump = list(dump_eeprom(programmer, size))
        filter_dump = list(filter(lambda x: x == int(arguments.erase, 0), dump))
        if len(filter_dump) != size:
            raise Exception('Validate EEPROM failed')

    elif len(arguments.write_byte) == 2:
        address = int(arguments.write_byte[0], 0).to_bytes(2, 'little')
        byte = int(arguments.write_byte[1], 0).to_bytes(1, 'little')
        programmer.write(b'\x03' + address + byte)
        ensure_ack(programmer)

    elif len(arguments.read_byte) == 2:
        address = int(arguments.read_byte, 0).to_bytes(2, 'little')
        programmer.write(b'\x04' + address + b'\x00')
        content = programmer.read(1)

        ensure_ack(programmer)
        print('%02X' % content[0])

    elif arguments.write_file is not None:
        file_size = os.path.getsize(arguments.write_file)
        if file_size > size:
            raise Exception('File can not bigger than %d' % size)

        file_size = file_size.to_bytes(2, 'little')
        programmer.write(b'\x05' + file_size + b'\x00')
        file = open(arguments.write_file, 'rb')
        programmer.write(file.read())

        ensure_ack(programmer)
    else:
        pass
    return 0


if __name__ == '__main__':
    main()
