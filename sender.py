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
c_socket.connect(s_host)

c_socket.sendall("Client".encode())
server_name = c_socket.recv(1024)
server_name = server_name.decode()
print('Connected with',server_name)

fptr = open('CS3543_100MB','rb')

while True:
    # message = input('Client : ')
    bytes_read = fptr.read(512 * 5)
    if not bytes_read:
        break
    c_socket.sendall(bytes_read.encode())
    # if message == 'close': #exit from loop if 'close' is encountered
    #     break
    message = (c_socket.recv(1)).decode()
    print(server_name,":",message)
    
c_socket.close()