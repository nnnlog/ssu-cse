import asyncio
import json
import random
import ssl
import memcache

import redis

logs = open("./server_log.txt", "a+", encoding="utf-8")  # Open a log file for appending logs

redis_client = redis.Redis(host='localhost', port=6379)  # Initialize the Redis client
print("Redis client starts on 127.0.0.1:6379")

memcache_client = memcache.Client(["127.0.0.1:11211"], debug=0)

# Initialize global variables for game state management
commonAnswer = -1
participants = 0
finishedParticipants = 0


def reset_game():
    global commonAnswer
    global participants
    global finishedParticipants

    commonAnswer = -1
    participants = finishedParticipants = 0  # Reset game state


class Server(asyncio.Protocol):
    cnt = 0
    mode = -1
    answer = -1
    multiJoined = False
    multiFinished = False

    def __init__(self):
        self.socket = None

    def key(self):
        a, b = self.socket.get_extra_info('socket').getpeername()
        return a + ":" + str(b)

    def connection_made(self, transport):
        self.socket = transport
        print(self.key())
        print("Client connected")  # Notify when a client connects
        memcache_client.set(self.key(), 0)  # reset points when new connect
        return

    def connection_lost(self, exc):
        global participants
        global commonAnswer
        global participants
        global finishedParticipants

        if self.multiJoined:
            participants -= 1  # Decrement participants count if in multiplayer mode
            if self.multiFinished:
                finishedParticipants -= 1  # Decrement finished participants count if the client finished the game
        return

    def data_received(self, data):
        global commonAnswer
        global participants
        global finishedParticipants

        data = json.loads(data.decode("ascii"))  # Decode and parse the received JSON data
        if "game_mode" in data:
            self.mode = data["game_mode"]  # Set the game mode based on client data
        elif "key" in data:
            fail = False
            key = data["key"]
            if self.mode == 1:
                answer = self.answer
                if key == self.answer:
                    # Correct guess in single-player mode
                    1
                else:
                    # Incorrect guess in single-player mode
                    self.cnt += 1
                    fail = True
            else:
                if commonAnswer == -1:
                    return  # No common answer set yet in multiplayer mode

                answer = commonAnswer
                if key == commonAnswer:
                    # Correct guess in multiplayer mode
                    1
                else:
                    self.cnt += 1
                    fail = True

            if not fail:
                memcache_client.incr(self.key(), 1)  # increase by one point.

                msg = json.dumps({"message": "Congratulations, you did it, start new round", "finish": True})
                self.socket.write(msg.encode("ascii"))  # Notify client of success
                redis_client.publish("game", json.dumps({"message": "Some client made answer, start new round."}))

                reset_game()  # Reset game after a successful guess
            else:
                if self.cnt == 5:
                    msg = json.dumps({"message": "Sorry, you've used all your attempts!", "finish": True})
                    self.socket.write(msg.encode("ascii"))  # Notify client of failure after maximum attempts

                    self.multiFinished = True
                    finishedParticipants += 1  # Update finished participants count

                    print(finishedParticipants, participants)
                    if finishedParticipants == participants:
                        redis_client.publish("game", json.dumps({"message": "All participants failed to guess, start new round."}))

                        reset_game()  # Reset game if all participants failed
                    return
                if key < answer:
                    msg = json.dumps({"message": "Hint: You guessed too small!", "finish": False})
                    self.socket.write(msg.encode("ascii"))  # Provide hint for guess being too low
                else:
                    msg = json.dumps({"message": "Hint: You guessed too high!", "finish": False})
                    self.socket.write(msg.encode("ascii"))  # Provide hint for guess being too high

        elif "start" in data:
            self.cnt = 0
            if self.mode == 1:
                self.answer = random.randint(1, 10)  # Set answer for single-player mode
            else:
                if commonAnswer == -1:
                    commonAnswer = random.randint(1, 10)  # Set common answer for multiplayer mode
                participants += 1
                self.multiJoined = True
                self.multiFinished = False

            msg = json.dumps({"message": "Rule Information:\n1. Number range: 1 ~ 10\n2. Maximum try per player: 5\n3. You can exit the multiplayer game during typing 'exit'."})
            self.socket.write(msg.encode("ascii"))  # Send game rules to the client
        elif "exit" in data:
            if self.multiJoined:
                participants -= 1  # Decrement participants count if the client exits
            if self.multiFinished:
                finishedParticipants -= 1  # Decrement finished participants count if the client had finished the game
            self.multiJoined = False
            self.multiFinished = False
            self.mode = -1  # Reset client's game mode


def make_tls_context():
    context = ssl.SSLContext(protocol=ssl.PROTOCOL_TLS_SERVER)
    context.load_cert_chain("cert.crt", "key.pem")
    context.check_hostname = False
    return context  # Create and configure SSL context for secure connections


loop = asyncio.get_event_loop()
coro = loop.create_server(Server, "127.0.0.1", 12345, ssl=make_tls_context())
server = loop.run_until_complete(coro)

print("Server listening on 127.0.0.1:12345")

try:
    loop.run_forever()
finally:
    server.close()
    loop.close()  # Start the server and run it until interrupted
