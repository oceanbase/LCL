from socket import *

HOST = '*.*.*.*'  # or 'localhost'
PORT = 1234
BUFSIZ = 50
ADDR = (HOST, PORT)

tcpCliSock = socket(AF_INET, SOCK_STREAM)
tcpCliSock.connect(ADDR)
print("these are valid commands:\n"
      "1.printG\n"
      "2.LCL_Once\n"
      "3.LCL_Periodic\n"
      "4.ALL_TO_DB_easy\n"
      "5.ALL_TO_DB")
while True:
    data1 = input('>')

    if not data1:
        tcpCliSock.close()
        break
    tcpCliSock.send(data1.encode())
    data1 = tcpCliSock.recv(BUFSIZ)
    print(data1.decode())

