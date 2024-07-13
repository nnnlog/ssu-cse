import pickle
import socket
import ssl
import json
import zlib

context = ssl.SSLContext(protocol=ssl.PROTOCOL_TLS_CLIENT)
context.load_verify_locations("cert.crt")
context.check_hostname = False

logs = open("./client_log.txt", "a+", encoding="utf-8")

games = open("./client_games.txt", "a+", encoding="utf-8")
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
    raw_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Set socket as TCP
    raw_sock.connect(("127.0.0.1", 12345))
    logs.write("[CONNECTION] %s" % "connected.\n")

    sock = context.wrap_socket(raw_sock)

    sock.sendall(b"start.")  # send "start." data
    logs.write("[SEND] %s" % "start.\n")

    curr_game = []

    for i in range(0, 5):
        print("Input a number (1 ~ 10): ", end="")

        try:
            s = int(input())
        except:
            logs.write("[CONNECTION] %s" % "disconnected.\n")
            logs.write("[ERROR] %s\n" % "invalid input")
            logs.close()
            print("Invalid input")
            exit(0)

        message = json.dumps({"key": s})
        logs.write("[SEND] %s\n" % message)
        curr_game.append("[SEND] %s" % message)

        sock.sendall(bytes(message, "ascii"))  # send server to current input number
        res = sock.recv(4096).decode("ascii")  # get respond from server
        logs.write("[RCVD] %s\n" % res)
        curr_game.append("[RCVD] %s" % res)
        if len(res) == 0:
            logs.write("[CONNECTION] %s" % "disconnected.\n")
            logs.write("[ERROR] %s\n" % "server error")
            logs.close()
            print("error")
            exit(0)

        res = json.loads(res)
        print(res["message"])

        if res["finish"]:  # Exit the program
            logs.write("[CONNECTION] %s" % "disconnected.\n")
            logs.close()

            encoded = pickle.dumps(curr_game)
            compressed = zlib.compress(encoded)
            game_data.append(compressed.hex())

            games.seek(0)
            games.truncate()
            games.write(json.dumps(game_data))
            games.flush()

            exit(0)

    res = sock.recv(4096).decode("ascii")  # get respond from server

    logs.write("[RCVD] %s\n" % res)
    curr_game.append("[RCVD] %s" % res)

    res = json.loads(res)
    print(res["message"])

    encoded = pickle.dumps(curr_game)
    compressed = zlib.compress(encoded)
    game_data.append(compressed.hex())

    games.seek(0)
    games.truncate()
    games.write(json.dumps(game_data))
    games.flush()
except socket.error as e:
    logs.write("[ERROR] %s\n" % e)
    print("detected error")
    print(e)

logs.write("[CONNECTION] %s" % "disconnected.\n")
logs.close()
