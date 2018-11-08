import socket
import struct
import threading

from server.models import ControlBoard

LED_SET_CMD = 0x79
LED_STATE = 0x7C

HOST_SRV = ''  # all interfaces
UDP_SRV_PORT = 8802

sock_rcv = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)  # UDP IPv6


def server():
    sock_rcv.bind((HOST_SRV, UDP_SRV_PORT))

    print("--- BOARD BRIDGE SOCKET SERVER STARTED ---")

    while True:
        data, addr = sock_rcv.recvfrom(1024)  # buffer size is 1024 bytes
        print("<<<< received message: {} from: [{}]: {}".format(data, addr[0].strip(), addr[1]))
        mac_part_ip_client = addr[0].strip()[-7:].strip(':')
        mac_part_ip_client = mac_part_ip_client[0:2] + ':' + mac_part_ip_client[2:4] + ':' \
                             + mac_part_ip_client[4:6] + ':' + mac_part_ip_client[6:]
        query = ControlBoard.objects().filter(mac_address__endswith=mac_part_ip_client)
        if query:
            board = query[0]
            offset = 0
            msg_map = '>BB'
            op = struct.unpack_from(msg_map, data, offset)
            offset += struct.calcsize(msg_map)
            cmd = op[0]
            value = op[1]
            if cmd == LED_STATE:
                print("Received LED_STATE with value {}".format(value))
                board.ipv6_address = addr[0].strip()
                board.last_led_level = value
                board.save()
            else:
                print("Invalid command received: {}".format(cmd))
        else:
            print("*** ERROR *** Control board not found with IP address {}. Ignoring message!".format(addr[0].strip()))


def run_server():
    threading.Thread(target=server).start()


def send_command(board: ControlBoard, value: int):
    buf = struct.pack('>BB', LED_SET_CMD, value)
    print(
        ">>>> sending to [{}]:{} - LED_SET_CMD({}): {}".format(board.ipv6_address, board.port_number, value, buf))
    sock_rcv.sendto(buf, (board.ipv6_address, board.port_number))
