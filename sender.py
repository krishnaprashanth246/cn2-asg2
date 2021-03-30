import socket
import sys
import struct
# from datetime import datetime

def header_def(seq_no, ack_no, ack, fin):
    header = struct.pack('ii??',source_ip, dest_ip, ack, fin)
    # header.fin
    return header

def rdt_send(data, addr, port):
    header = header_def(0, 0, 0, 0)
    total_packets = 1024*100
    timer = 0
    



# Creating server socket
c_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
s_PORT   = 9999

# if(len(sys.argv) != 2):
# 	print("format: <filename> <server name>")
# 	sys.exit()

name = sys.argv[1]
allhostinfo = socket.getaddrinfo(name, s_PORT)
# print(allhostinfo)
hostinfo, _, _ = allhostinfo
s_host = hostinfo[4]

fptr = open('CS3543_100MB','rb')
data = fptr.read(1024*1024*100)

rdt_send(data,s_host[0],s_host[1])
#fptr = open('tempfile','rb')
# bytes_sent = 0
while True:
    # message = input('Client : ')
    bytes_read = fptr.read(1024)
    if not bytes_read:
        break
    # bytes_sent = bytes_sent + c_socket.sendto(bytes_read, s_host)
# print(bytes_sent/1024)