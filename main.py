import argparse

import serial as Serial


def ensure_ack(ser: Serial.Serial):
    ack = ser.readline().decode('ascii').strip()
    if ack != 'ACK':
        raise Exception('reply not ack')


def dump_eeprom(ser: Serial.Serial, sz):
    ser.write(b'\x01\x00\x00\x00')
    eeprom_content = ser.read(sz)
    ensure_ack(ser)
    return eeprom_content


def make_connection(port):
    ser = Serial.Serial(port, baudrate=57600)
    ensure_ack(ser)
    sz = int(ser.readline().decode('ascii').strip())
    return ser, sz


if __name__ == '__main__':
    parser = argparse.ArgumentParser(usage='An EEPROM programmer')

    parser.add_argument('-d', '--dump', help='dump current EEPROM content', action='store_true')
    parser.add_argument('-p', '--port', type=str, metavar='PORT', help='Serial port', required=True)
    parser.add_argument('-e', '--erase', nargs='?', type=str, metavar='BYTE',
                        help='Fill byte to EEPROM')
    parser.add_argument('-w', '--write', type=str, metavar='PATH', help='Write a file to EEPROM')
    parser.add_argument('-s', '--single', nargs=2, type=str, help='Write a single byte to EEPROM')

    arguments = parser.parse_args()

    print('Connecting to device ... ')
    programmer, size = make_connection(arguments.port)
    if arguments.dump:

        dump = dump_eeprom(programmer, size)

        print('Total %d' % len(dump))
        for base in range(0, len(dump), 16):
            output = '%04X    ' % base
            for offset in range(0, 16, 1):
                output += '%02X ' % dump[base + offset]
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
    elif len(arguments.single) == 2:
        address = int(arguments.single[0], 0).to_bytes(2, 'little')
        byte = int(arguments.single[1], 0).to_bytes(1, 'little')
        programmer.write(b'\x03' + address + byte)

        ensure_ack(programmer)

    else:
        pass
