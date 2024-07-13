import socket

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.bind(("127.0.0.1", 12345))  # bind to 127.0.0.1:12345

print("Server listening on 127.0.0.1:12345")

while True:
    data, address = sock.recvfrom(4096)
    text = data.decode("ascii")

    for k in ["a", "e", "i", "o", "u"]:
        text = ("%s%s" % (k, k)).join(text.split(k))  # make double count of a, e, i, o, u

    print("Sent modified message back to ('%s', %d): %s" % (address[0], address[1], text))
    sock.sendto(bytes(text, "ascii"), address)  # send client to modified message
