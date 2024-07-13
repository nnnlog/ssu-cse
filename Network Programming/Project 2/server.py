import json
import random
import socket
import ssl
import pickle
import zlib

logs = open("./server_log.txt", "a+", encoding="utf-8")

games = open("./server_games.txt", "a+", encoding="utf-8")
games.seek(0)
game_data = []
msg = games.read()

if len(msg) > 0:
    game_data = json.loads(msg)

print("Showing previous game data log...")
for i, game in enumerate(game_data):
    print("%dth game data:" % i)
    data = pickle.loads(zlib.decompress(bytes.fromhex(game)))
    for msg in data:
        print(msg)
    print()
print()

try:
    sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    sock.bind(("127.0.0.1", 12345))
    sock.listen(1)

    context = ssl.SSLContext(protocol=ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain("cert.crt", "key.pem")
    context.check_hostname = False

    print("Server listening on 127.0.0.1:12345")
    logs.write("[CONNECTION] %s" % "service started.\n")

    while True:
        raw_sock, _ = sock.accept()
        sc = context.wrap_socket(raw_sock, server_side=True)

        curr_game = []

        logs.write("[CONNECTION] %s" % "socket accepted.\n")
        msg = sc.recv(4096)
        logs.write("[RECV] %s\n" % msg)
        curr_game.append("[RECV] %s" % msg)

        if msg == b"start.":
            x = random.randint(1, 10)  # generate random int
            logs.write("[GAME] Set initial number to %s\n" % x)

            for i in range(0, 5):
                try:
                    msg = sc.recv(4096).decode("ascii")
                    logs.write("[RECV] %s\n" % msg)
                    curr_game.append("[RECV] %s" % msg)
                    guess = json.loads(msg)['key']  # get current query number
                    assert type(guess) == int
                except:
                    logs.write("[ERROR] %s\n" % "server error")
                    print("error")
                    break

                if i + 1 == 5:
                    msg = json.dumps({"message": "Sorry, you've used all your attempts!", "finish": True})
                    curr_game.append("[SEND] %s" % msg)
                    logs.write("[SEND] %s\n" % msg)
                    sc.sendall(msg.encode("ascii"))
                    break

                if guess == x:
                    msg = json.dumps({"message": "Congratulations, you did it.", "finish": True})
                    curr_game.append("[SEND] %s" % msg)
                    logs.write("[SEND] %s\n" % msg)
                    sc.sendall(msg.encode("ascii"))
                    break
                elif guess < x:
                    msg = json.dumps({"message": "Hint: You guessed too small!", "finish": False})
                    curr_game.append("[SEND] %s" % msg)
                    logs.write("[SEND] %s\n" % msg)
                    sc.sendall(msg.encode("ascii"))
                else:
                    msg = json.dumps({"message": "Hint: You guessed too high!", "finish": False})
                    curr_game.append("[SEND] %s" % msg)
                    logs.write("[SEND] %s\n" % msg)
                    sc.sendall(msg.encode("ascii"))

        encoded = pickle.dumps(curr_game)
        compressed = zlib.compress(encoded)
        game_data.append(compressed.hex())

        games.seek(0)
        games.truncate()
        games.write(json.dumps(game_data))
        games.flush()

        sc.close()  # close socket
        logs.write("[CONNECTION] %s" % "close socket\n")
except socket.error as e:
    print("Detected Socket error")
    logs.write("[ERROR] %s\n" % "socket error")
    print(e)
