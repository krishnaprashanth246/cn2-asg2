# Imports
import socket
import time
import sys
from datetime import datetime

MAX_CLIENTS = 10

s_soc = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # creating socket

port = 9999

s_soc.bind((b'',port)) #binding to port

fileptr = open('receivedfile', 'wb')
while True:
    data, addr = s_soc.recvfrom(1024)
    if not data:
    	break
    fileptr.write(data)
   