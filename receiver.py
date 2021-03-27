# Imports
import socket
import time
import sys
from datetime import datetime

MAX_CLIENTS = 10

s_soc = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # creating socket

port = 9999

s_soc.bind((b'',port)) #binding to port


name = input('Enter name of Server:')
s_soc.listen(MAX_CLIENTS)

conn, add = s_soc.accept() # accepting connection
#print(conn) 
print('Connection Established. Connected From:',add[0])


#  Storing incoming connection data

client = (conn.recv(1024)).decode() 
print("Connection etablished with :",client)
""" 
details of incoming connection stored.
clients name max size = 1024 and it is decoded on the server
"""
conn.sendall(name.encode()) # send hostname to client

while True:
    #message = input('Server:')
    #conn.sendall(message.encode())
    message = conn.recv(1024)
    message = message.decode()
    print(client,':',message)
    if message == 'close': #close the server if 'close' is encountered
        break;
    conn.sendall(message.encode())
conn.close()