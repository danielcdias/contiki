import random
import socket
import struct

LED_TOGGLE_REQUEST = 0x79
LED_SET_STATE = 0x7A
LED_GET_STATE = 0x7B
LED_STATE = 0x7C

LEDS_OFF = 0
LEDS_RED = 1
LEDS_GREEN = 2
LEDS_ALL = 3

LedsState = (
    (LEDS_OFF, "OFF"),
    (LEDS_RED, "RED"),
    (LEDS_GREEN, "GREEN"),
    (LEDS_ALL, "ALL"),
)

HOST_SRV = ''  # all interfaces
UDP_SRV_PORT = 8802

def get_cmd_name(cmd: int):
    cmd_name = 'INVALID'
    if cmd == LED_TOGGLE_REQUEST:
        cmd_name = 'LED_TOGGLE_REQUEST'
    if cmd == LED_SET_STATE:
        cmd_name = 'LED_SET_STATE'
    if cmd == LED_GET_STATE:
        cmd_name = 'LED_GET_STATE'
    if cmd == LED_STATE:
        cmd_name = 'LED_STATE'
    return cmd_name


def main():
    sock_rcv = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)  # UDP IPv6
    sock_rcv.bind((HOST_SRV, UDP_SRV_PORT))

    while True:
        print('------------------------------------------------------------------------------')
        data, addr = sock_rcv.recvfrom(1024)  # buffer size is 1024 bytes
        print("<<<< received message: {} from: [{}]: {}".format(data, addr[0].strip(), addr[1]))
        offset = 0
        msg_map = ''
        if len(data) == 1:
            msg_map = '>B'
        else:
            msg_map = '>BB'
        op = struct.unpack_from(msg_map, data, offset)
        offset += struct.calcsize(msg_map)
        cmd = op[0]
        value = op[1] if len(data) == 2 else -1
        if cmd == LED_STATE:
            print("Received status: {} with value {}".format(get_cmd_name(cmd), LedsState[value][1]))
        elif cmd == LED_TOGGLE_REQUEST:
            print("LED_TOGGLE_REQUEST command received.")
            buf = struct.pack('>BB', LED_SET_STATE, random.randint(0, 3))
            print(
                ">>>> sending to [{}]:{} - LED_SET_STATE({}): {}".format(addr[0].strip(), addr[1], LedsState[buf[1]][1],
                                                                         buf))
            sock_rcv.sendto(buf, (addr[0].strip(), addr[1]))
        else:
            print("*** ERROR *** Invalid data received!")


if __name__ == '__main__':
    main()
