import socket
import sys
from datetime import datetime

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
#fptr = open('tempfile','rb')

while True:
    # message = input('Client : ')
    bytes_read = fptr.read(1024)
    if not bytes_read:
        break
    c_socket.sendto(bytes_read, s_host)
   