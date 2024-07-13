import random
import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
sock.bind(("127.0.0.1", 12345))
sock.listen(1)

print("Server listening on 127.0.0.1:12345")

while True:
    sc, sockname = sock.accept()

    msg = sc.recv(4096)

    if msg == b"start.":
        x = random.randint(1, 10)  # generate random int

        for i in range(0, 5):
            guess = int(sc.recv(4096).decode("ascii"))  # get current query number

            if guess == x:
                sc.sendall(b"Congratulations you did it.")
                break
            elif guess < x:
                sc.sendall(b"You guessed too small!")
            else:
                sc.sendall(b"You guessed too high!")

    sc.close()  # close socket
