import socket
import struct
import random
import math

HOST = ''  # all interfaces
UDP_PORT_RCV = 8802

sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)  # UDP
sock.bind((HOST, UDP_PORT_RCV))

LED_TOGGLE_REQUEST = (0x79)
LED_SET_STATE = (0x7A)
LED_GET_STATE = (0x7B)
LED_STATE = (0x7C)
OP_REQUEST = (0x6E)
OP_RESULT = (0x6F)

OP_MULTIPLY = (0x22)
OP_DIVIDE = (0x23)
OP_SUM = (0x24)
OP_SUBTRACT = (0x25)

LED_GREEN = 0x1
LED_RED = 0x2
LED_BLUE = 0x4
LED_YELLOW = 0x8


def operate(OP1, OP2, operacao, Fc):
    if operacao == OP_MULTIPLY:
        val = OP1 * OP2
    elif operacao == OP_DIVIDE:
        val = OP1 / OP2
    elif operacao == OP_SUM:
        val = OP1 + OP2
    elif operacao == OP_SUBTRACT:
        val = OP1 - OP2

    return (val * Fc)


def calc_checksum(s):
    sum = 0
    for c in s:
        sum += ord(c)
    # sum = -(sum % 256)
    return '%2X' % ((sum % 256) & 0xFF)


def printled(state):
    if (state & LED_GREEN) != 0:
        print("GREEN IS ON")
    else:
        print("GREEN IS OFF")

    if (state & LED_RED) != 0:
        print("RED IS ON")
    else:
        print("RED IS OFF")

    if (state & LED_BLUE) != 0:
        print("BLUE IS ON")
    else:
        print("BLUE IS OFF")

    if (state & LED_YELLOW) != 0:
        print("YELLOW IS ON")
    else:
        print("YELLOW IS OFF")


def printop(op):
    if op == OP_DIVIDE:
        return '/'
    elif op == OP_MULTIPLY:
        return '*'
    elif op == OP_SUBTRACT:
        return '-'
    elif op == OP_SUM:
        return '+'
    else:
        return '?'


while True:
    data, addr = sock.recvfrom(1024)  # buffer size is 1024 bytes
    print("received message: from: [", addr[0].strip(), "]:", addr[1])
    offset = 0
    # op = struct.unpack(">B", data)
    op = struct.unpack_from(">B", data, offset)
    offset = struct.calcsize(">B")

    if op[0] == LED_TOGGLE_REQUEST:
        print("RECEIVED LED_TOGGLE_REQUEST PORT (", addr[1], ") SEND LED_TOGGLE TO [", addr[0].strip(), "]:", addr[1])
        stt = random.randint(0, 3)
        msg = struct.pack(">BB", LED_SET_STATE, stt)
        # printled(stt)
        # sndsock = socket.socket(socket.AF_INET6,socket.SOCK_DGRAM)
        # sndsock.bind((addr[0].strip(), UDP_PORT))
        sock.sendto(msg, (addr[0].strip(), addr[1]))
        # sndsock.close()
        # sock.bind((HOST, UDP_PORT))

    if op[0] == LED_STATE:
        state = struct.unpack_from(">B", data, offset)
        printled(state[0])

    if op[0] == OP_REQUEST:
        print("RECEIVED OP_REQUEST PORT (", addr[1], ") SEND OP_RESULT TO [", addr[0].strip(), "]:", addr[1])
        cmd, OP1, operacao, OP2, Fc = struct.unpack_from("<BiBif", data, 0)
        # for character in data:
        #    print('0x',character.encode('hex'))
        fp_res = operate(OP1, OP2, operacao, Fc)
        i_res_frac, i_res = math.modf(fp_res)
        msg = struct.pack("<Biif", OP_RESULT, int(i_res), int(i_res_frac * 10000), fp_res)
        print("Send (", OP1, printop(operacao), OP2, ")*", Fc, "=", fp_res, " to ", addr[0].strip(), "port ", addr[1])
        msg = msg + calc_checksum(msg)
        sock.sendto(msg, (addr[0].strip(), addr[1]))
