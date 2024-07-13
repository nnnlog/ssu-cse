import socket
import json

sock = socket.create_connection(("ip-api.com", 80))
sock.send(b"GET /json/?fields=country,regionName,city,lat,lon,query&lang=fr HTTP/1.1\r\n"
          b"Host: ip-api.com\r\n"
          b"\r\n")  # send raw http request(GET method) packet
raw = sock.recv(4096)  # receive response
body = raw.split(b"\r\n\r\n")[1]  # Response data structure: {header}\r\n\r\n{body}
json = json.loads(body)

for k in ["query", "country", "regionName", "city", "lat", "lon"]:
    print("%s = %s" % (k, json[k]))
