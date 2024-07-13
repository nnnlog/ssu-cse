import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # set socket as udp protocol

print("Enter message to send to server: ", end="")

s = input()

sock.sendto(bytes(s, "ascii"), ("127.0.0.1", 12345))  # send data that inputs from user

data, address = sock.recvfrom(4096)  # receive modified message from server
text = data.decode("ascii")

print("Received modified message from server: %s" % text)
