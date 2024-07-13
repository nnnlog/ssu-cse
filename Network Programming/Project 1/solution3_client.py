import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Set socket as TCP
sock.connect(("127.0.0.1", 12345))
sock.sendall(b"start.")  # send "start." data

for i in range(0, 5):
    print("Input a number (1 ~ 10): ", end="")
    s = int(input())
    sock.sendall(bytes(str(s), "ascii"))  # send server to current input number
    res = sock.recv(4096).decode("ascii")  # get respond from server
    print(res)

    if res == "Congratulations you did it.":  # Exit the program
        exit(0)

print("failed to guess number.")
