import socket
import struct
import time

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

UDP_PORT = 8802
UDP_IP = "::1"

HOST_RCV = ''  # all interfaces
UDP_RCV_PORT = 3000


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
    current_state = LEDS_OFF

    sock_rcv = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)  # UDP IPv6
    sock_rcv.bind((HOST_RCV, UDP_RCV_PORT))

    while True:

        buf = struct.pack('>BB', LED_TOGGLE_REQUEST, 0)

        print(">>>> sending message: {}".format(buf))

        sock_snd = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
        sock_snd.sendto(buf, (UDP_IP, UDP_PORT))

        data, addr = sock_rcv.recvfrom(1024)  # buffer size is 1024 bytes
        print("<<<< received message: {} from: [{}]: {}".format(data, addr[0].strip(), addr[1]))
        offset = 0
        op = struct.unpack_from(">BB", data, offset)
        offset += struct.calcsize(">BB")
        cmd = op[0]
        value = op[1]
        print("Received command: {} with value {}".format(get_cmd_name(cmd), LedsState[value][1]))

        if cmd == LED_SET_STATE or cmd == LED_GET_STATE:
            if cmd == LED_SET_STATE:
                current_state = value
            print(">>>> sending reply: {}".format(buf))
            reply = struct.pack('>BB', LED_STATE, current_state)
            sock_snd.sendto(reply, (UDP_IP, UDP_PORT))
        else:
            print("*** ERROR *** Invalid command received.")

        time.sleep(3)


if __name__ == '__main__':
    main()
