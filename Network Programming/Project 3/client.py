import itertools
import json
import multiprocessing
import socket
import ssl
import threading
import time
from queue import Queue

import redis

# Set up SSL context for secure client connection
context = ssl.SSLContext(protocol=ssl.PROTOCOL_TLS_CLIENT)
context.load_verify_locations("cert.crt")
context.check_hostname = False

logs = open("./client_log.txt", "a+", encoding="utf-8")  # Open a log file for appending logs


def listen(q):
    redis_client = redis.Redis(host='localhost', port=6379)  # Initialize the Redis client
    pubsub = redis_client.pubsub()
    pubsub.subscribe('game')  # Subscribe to the 'game' channel

    gen = pubsub.listen()
    for i in gen:
        if i["data"] == 1:
            continue  # Ignore non-message data
        q.put(i)  # Put the received message into the queue
        return


def select_game_mode():
    while True:
        try:
            print("Select game mode (1: single, 2: multi) : ", end="")
            s = input()  # Prompt user to select game mode
            if s == "exit":
                exit(0)
            s = int(s)
            assert 1 <= s <= 2  # Ensure valid game mode
            return s
        except:
            continue  # Repeat prompt on invalid input

try:
    raw_sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)  # Set socket as TCP
    raw_sock.connect(("127.0.0.1", 12345))  # Connect to the server
    logs.write("[CONNECTION] %s" % "connected.\n")

    sock = context.wrap_socket(raw_sock)  # Wrap the socket with SSL
    continueGame = False

    while True:
        if not continueGame:
            game_mode = select_game_mode()  # Select the game mode

        while True:
            message = json.dumps({"game_mode": game_mode})
            sock.sendall(bytes(message, "ascii"))  # Send the game mode to the server
            logs.write("[SEND] %s\n" % message)
            break

        message = json.dumps({"start": True})
        sock.sendall(bytes(message, "ascii"))  # Send start message to the server

        res = sock.recv(4096).decode("ascii")  # Receive the server response
        res = json.loads(res)
        print(res["message"])  # Print the server response

        q = Queue()  # Initialize a queue for inter-thread communication
        thread1 = threading.Thread(target=listen, args=(q,))  # Create a listening thread

        thread1.start()  # Start the listening thread

        continueGame = True
        killedByGen = False

        for i in range(0, 6):
            if game_mode == 2 and not q.empty():
                nxt = q.get()["data"]  # Check for messages from the server
                nxt = json.loads(nxt)["message"]
                print(nxt)
                killedByGen = True
                break

            print("Input a number (1 ~ 10): ", end="")

            s = input()  # Prompt user to input a number

            if game_mode == 2 and not q.empty():
                nxt = q.get()["data"]
                nxt = json.loads(nxt)["message"]
                print(nxt)
                killedByGen = True
                break

            try:
                s = int(s)
            except:
                if s == "exit":
                    continueGame = False
                    message = json.dumps({"exit": True})
                    sock.sendall(bytes(message, "ascii"))  # Send exit message to the server
                    killedByGen = True
                    break
                logs.write("[CONNECTION] %s" % "disconnected.\n")
                logs.write("[ERROR] %s\n" % "invalid input")
                logs.close()
                print("Invalid input")
                exit(0)

            message = json.dumps({"key": s})
            logs.write("[SEND] %s\n" % message)

            sock.sendall(bytes(message, "ascii"))  # Send the user's guess to the server
            res = sock.recv(4096).decode("ascii")  # Receive the server response
            logs.write("[RCVD] %s\n" % res)
            if len(res) == 0:
                logs.write("[CONNECTION] %s" % "disconnected.\n")
                logs.write("[ERROR] %s\n" % "server error")
                logs.close()
                print("error")
                exit(0)

            res = json.loads(res)
            print(res["message"])  # Print the server response

            if res["finish"]:  # Check if the current round has finished
                break

        logs.flush()

        while not killedByGen and game_mode == 2 and q.empty():
            time.sleep(0.5)  # Wait for messages from the server

        if not killedByGen and game_mode == 2 and not q.empty():
            nxt = q.get()["data"]
            nxt = json.loads(nxt)["message"]
            print(nxt)  # Print messages from the server

except socket.error as e:
    logs.write("[ERROR] %s\n" % e)
    print("detected error")
    print(e)  # Handle socket errors
