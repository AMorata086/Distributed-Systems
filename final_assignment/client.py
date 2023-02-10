from enum import Enum
import argparse
import threading
import socket


class Request:
    # Will work for register and unregister but they will have different opcode
    def registration(self, opcode, userSender):
        self.opcode = opcode + "\0"
        self.userSender = userSender + "\0"

    def connect(self, opcode, userSender, clientListeningPort):
        self.opcode = opcode + "\0"
        self.userSender = userSender + "\0"
        self.clientListeningPort = clientListeningPort + "\0"

    # Repeated but works for naming
    def disconnect(self, opcode, userSender):
        self.opcode = opcode + "\0"
        self.userSender = userSender + "\0"

    def messagePassing(self, opcode, userSender, userDestination, msg):
        self.opcode = opcode + "\0"
        self.userSender = userSender + "\0"
        self.userDestination = userDestination + "\0"
        self.msg = msg + "\0"


def threaded_listener(mysocket):
    # 9 connections are kept waiting if the server is busy and if a 10th socket tries to connect then the connection is refused.
    mysocket.listen(9)
    print("Socket for receiving messages for client is listening")

    while True:
        # Wait for a connection
        print("Waiting for a connection ...")
        connection, client_address = mysocket.accept()
        try:
            print('Connection from ', client_address)
            # Receive the data in small chunks
            while True:
                # ?????????????????????????? HOW MANY HAY QUE VER CUANTOS BYTES Y COMO LOS RECIBIMOS Y PROCESAMOS
                data = connection.recv(1)
                if (data == b'\0'):
                    break
                message += data.decode()
                # print("MESSAGE " + + " FROM " + username +
                #   ": \n" + message + "\n END")
        except socket.error:
            print("There has been an error with the socket listening")


class client:

    # ******************** TYPES *********************
    # *
    # * @brief Return codes for the protocol methods
    class RC(Enum):
        OK = 0
        ERROR = 1
        USER_ERROR = 2

    # ****************** ATTRIBUTES ******************
    _server = ""        # Server IP. It will be inputed by user
    _port = "40000"     # Server Port. It will be inputed by user

    _username = ""  # We will keep track of the username for those methods in which you need the receiver user and the user sending it

    # ****************** SOCKET AND THREADING ******************
    # Our client will also act as a server since it needs to receive messages once connected. Therefore, we need to assign its
    # IP and port to its own socket
    # Server part of the client IP address and Port
    _hostIP = "127.0.0.1"
    _hostPort = 12345

    server_address = (_hostIP, _hostPort)

    # Create server socket instance for the client
    server_socket = socket.socket()
    try:
        # With the help of bind() function
        # binding host and port
        server_socket.bind(server_address)
    except socket.error as message:
        print("SOCKET ERROR IN LISTENING PORT FOR CLIENT")

    # Invoking of the threaded function for listening for incoming messages in the client-side (server part). It listens on another port
    client_listening = threading.Thread(
        target=threaded_listener, args=(server_socket,))

    # ******************** METHODS *******************
    # *
    # * @param user - User name to register in the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user is already registered
    # * @return ERROR if another error occurred

    @ staticmethod
    def register(user):

        # Creation of the socket and connection to the server
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((client._server, client._port))

        # Initialization of the request with only the fields needed. In the
        # Request constructor the checks will be performed and \0 will be added
        # register_request = Request.registration(client, "REGISTER", user)

        # First get the attribute of the operation code and then encode it to send
        # it over the socket
        # opcode = (getattr(register_request, 'opcode')).encode()
        # s.send(opcode)

        s.send("REGISTER\0".encode())
        user = str(user + "\0")
        s.send(user.encode())

        # s.send((str(user + "\0")).encode())

        # It's not repetitive since in the constructor username will be checked
        # username = (getattr(register_request, 'userSender')).encode()
        # s.send(username)

        # We receive a response through the socket from the server. It just receives a byte that encodes the result of the operation
        data = s.recv(1).decode()

        # After receiving the operation code from server, closing of the socket
        s.close()

        # The server will send chars so the comparison will be done with strings
        if data == "0":
            print("[REGISTER] User successfully registered")
            return client.RC.OK
        elif data == "1":
            print("[REGISTER - ERROR] User has been previously registered")
            return client.RC.USER_ERROR
        else:  # For security, we don't only compare it to "2"
            print(
                "[REGISTER - ERROR] There has been an unknown error with the registration")
            return client.RC.ERROR

    # *
    # 	 * @param user - User name to unregister from the system
    # 	 *
    # 	 * @return OK if successful
    # 	 * @return USER_ERROR if the user does not exist
    # 	 * @return ERROR if another error occurred

    @ staticmethod
    def unregister(user):

        # Creation of the socket and connection to the server
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((client._server, client._port))

        # Initialization of the request with only the fields needed
        # unregister_request = Request.registration("UNREGISTER", user)

        # First get the attribute of the operation code and then encode it to send
        # it over the socket
        # opcode = (getattr(unregister_request, 'opcode')).encode()
        # s.send(opcode)

        # # It's not repetitive since in the constructor username will be checked
        # username = (getattr(unregister_request, 'userSender')).encode()
        # s.send(username)
        s.send("UNREGISTER\0".encode())
        user = str(user + "\0")
        s.send(user.encode())

        # We receive a response through the socket from the server. It just receives a byte that encodes the result of the operation
        data = s.recv(1).decode()

        # After receiving the operation code from server, closing of the socket
        s.close()

        if data == "0":  # Dice un BYTE, STRING O NUMERO? COMPROBAR
            print("[UNREGISTER] User successfully unregistered")
            return client.RC.OK
        elif data == "1":
            print("[UNREGISTER - ERROR] User doesn't exist")
            return client.RC.USER_ERROR
        else:  # For security, we don't only compare it to "2"
            print(
                "[UNREGISTER - ERROR] There's been an unknown error with the unregistration")
            return client.RC.ERROR

    # *
    # * @param user - User name to connect to the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist or if it is already connected
    # * @return ERROR if another error occurred

    @ staticmethod
    def connect(user):
        # We assign the variable of client self username
        client._username = user

        # Creation of the socket to connect to the server
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((client._server, client._port))

        # Initialization of the request with only the fields needed
        # connection_request = Request.connect(
        #     "CONNECT", user, client._hostPort)

        # First get the attribute of the operation code and then encode it to send
        # it over the socket
        # opcode = (getattr(connection_request, 'opcode')).encode()
        # s.send(opcode)

        # It's not repetitive since in the constructor username will be checked
        # username = (getattr(connection_request, 'userSender')).encode()
        # s.send(username)

        # Sending of the user port
        # userport = (getattr(connection_request,
        #             'clientListeningPort')).encode()
        # s.send(userport)
        s.send("CONNECT\0".encode())
        user = str(user + "\0")
        s.send(user.encode())

        portToSend = str(client._hostPort) + "\0"
        s.send(portToSend.encode())

        # We receive a response through the socket from the server. It just receives a byte that encodes the result of the operation
        data = s.recv(1).decode()

        # After receiving the operation code from server, closing of the socket
        s.close()

        # Now starting the client listening for incoming requests after connecting
        client.client_listening.start()

        if data == "0":
            print("[CONNECT] User successfully connected")
            return client.RC.OK
        elif data == "1":
            print("[CONNECT - ERROR] User does not exist")
            return client.RC.USER_ERROR
        elif data == "2":
            print("[CONNECT - ERROR] User is already connected")
            return client.RC.USER_ERROR
        else:  # For security, we don't only compare it to "3"
            print("[CONNECT - ERROR] There's been an unknown error with the connection")
            return client.RC.ERROR

    # *
    # * @param user - User name to disconnect from the system
    # *
    # * @return OK if successful
    # * @return USER_ERROR if the user does not exist
    # * @return ERROR if another error occurred

    @staticmethod
    def disconnect(user):

        # Creation of the socket and connection to the server
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((client._server, client._port))

        # Initialization of the request with only the fields needed
        #disconnection_request = Request.disconnect("DISCONNECT", user)

        # First get the attribute of the operation code and then encode it to send
        # it over the socket
        #opcode = (getattr(disconnection_request, 'opcode')).encode()
        # s.send(opcode)

        # It's not repetitive since in the constructor username will be checked
        #username = (getattr(disconnection_request, 'userSender')).encode()
        # s.send(username)

        s.send(("DISCONNECT\0").encode())

        user = str(user + "\0")
        s.send(user.encode())

        portToSend = str(client._hostPort) + "\0"
        s.send(portToSend.encode())

        # We receive a response through the socket from the server. It just receives a byte that encodes the result of the operation
        data = s.recv(1).decode()

        # After receiving the operation code from server, closing of the socket
        s.close()

        # Unassignan the client username since now its disconnected
        client._username = ""

        # Thread and socket hygiene:
        # ERROR HANDLING AQUI ---------------------------------------------------
        client.client_listening.join()  # Joining the thread
        client.server_socket.close()    # Closing the socket closes the connection

        if data == "0":
            print("[DISCONNECT] User successfully disconnected")
            return client.RC.OK
        elif data == "1":
            print("[DISCONNECT - ERROR] User doesn't exist")
            return client.RC.USER_ERROR
        elif data == "2":
            print("[DISCONNECT - ERROR] User is not connected")
            return client.RC.USER_ERROR
        else:  # For security, we don't only compare it to "3"
            print("[DISCONNECT - ERROR] There's been an error with user registration")
            return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param message - Message to be sent
    # *
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def send(user,  message):

        # Creation of the socket and connection to the server
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((client._server, client._port))

        # Initialization of the request with only the fields needed
        msgsend_request = Request.messagePassing(
            "SEND", client._username, user, message)

        # First get the attribute of the operation code and then encode it to send
        # it over the socket
        opcode = (getattr(msgsend_request, 'opcode')).encode()
        s.send(opcode)

        # COMPROBAR ESTO MEJOR 23423423423423423423234234234023423423
        # We add the end of string symbol (\0) and encode it to send
        usernameSend = (user + "\0").encode()
        s.send(usernameSend)

        # It's not repetitive since in the constructor username will be checked
        usernameDest = (getattr(msgsend_request, 'userDestination')).encode()
        s.send(usernameDest)

        # Lastly, we send the message
        msg = (getattr(msgsend_request, 'msg')).encode()
        s.send(msg)

        # We ll receive a short and an int. We stop receiving when there's a \0
        while True:
            data = s.recv(1)
            if (data == b'\0'):
                break
            message += data.decode()

        # PYTHON COGER LA POSICION 0 (SERA MI OP CODE QUE ENVIARE DEL SERVER SIDE) EL RESTO DE NUMEROS SERAN EL MSG ID
        # EL RESTO LO COJO Y LO PRINTEO
        # Closing of the socket
        s.close()
        if data == "0":
            print("[SEND] Message has sucessfully been sent")
            print(str(data))
            return client.RC.OK

        elif data == "1":
            print("[SEND - ERROR] User doesn't exist")
            return client.RC.USER_ERROR
        else:  # For security, we don't only compare it to "2"
            print("[DISCONNECT - ERROR] There has been an unknown error")
            return client.RC.ERROR

    # *
    # * @param user    - Receiver user name
    # * @param file    - file  to be sent
    # * @param message - Message to be sent
    # *
    # * @return OK if the server had successfully delivered the message
    # * @return USER_ERROR if the user is not connected (the message is queued for delivery)
    # * @return ERROR the user does not exist or another error occurred
    @staticmethod
    def sendAttach(user,  file,  message):
        #  Write your code here
       # Creation of the socket and connection to the server
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((client._server, client._port))

        # Initialization of the request with only the fields needed

        # We send the message over the socket
        s.send()

        # LAST THING. We receive a response through the socket from the server
        s.recv()

        # Closing of the socket
        s.close()
        return client.RC.ERROR

    # *
    # **
    # * @brief Command interpreter for the client. It calls the protocol functions.
    @staticmethod
    def shell():

        while (True):
            try:
                command = input("c> ")
                line = command.split(" ")
                if (len(line) > 0):

                    line[0] = line[0].upper()

                    if (line[0] == "REGISTER"):
                        if (len(line) == 2):
                            client.register(line[1])
                        else:
                            print("Syntax error. Usage: REGISTER <userName>")

                    elif(line[0] == "UNREGISTER"):
                        if (len(line) == 2):
                            client.unregister(line[1])
                        else:
                            print("Syntax error. Usage: UNREGISTER <userName>")

                    elif(line[0] == "CONNECT"):
                        if (len(line) == 2):
                            client.connect(line[1])
                        else:
                            print("Syntax error. Usage: CONNECT <userName>")

                    elif(line[0] == "DISCONNECT"):
                        if (len(line) == 2):
                            client.disconnect(line[1])
                        else:
                            print("Syntax error. Usage: DISCONNECT <userName>")

                    elif(line[0] == "SEND"):
                        if (len(line) >= 3):
                            #  Remove first two words
                            message = ' '.join(line[2:])
                            client.send(line[1], message)
                        else:
                            print("Syntax error. Usage: SEND <userName> <message>")

                    elif(line[0] == "SENDATTACH"):
                        if (len(line) >= 4):
                            #  Remove first two words
                            message = ' '.join(line[3:])
                            client.sendAttach(line[1], line[2], message)
                        else:
                            print(
                                "Syntax error. Usage: SENDATTACH <userName> <filename> <message>")

                    elif(line[0] == "QUIT"):
                        if (len(line) == 1):
                            break
                        else:
                            print("Syntax error. Use: QUIT")
                    else:
                        print("Error: command " + line[0] + " not valid.")
            except Exception as e:
                print("Exception: " + str(e))

    # *
    # * @brief Prints program usage
    @staticmethod
    def usage():
        print("Usage: python3 client.py -s <server> -p <port>")

    # *
    # * @brief Parses program execution arguments

    @staticmethod
    def parseArguments(argv):
        parser = argparse.ArgumentParser()
        parser.add_argument('-s', type=str, required=True, help='Server IP')
        parser.add_argument('-p', type=int, required=True, help='Server Port')
        args = parser.parse_args()

        if (args.s is None):
            parser.error("Usage: python3 client.py -s <server> -p <port>")
            return False

        if ((args.p < 1024) or (args.p > 65535)):
            parser.error(
                "Error: Port must be in the range 1024 <= port <= 65535")
            return False

        client._server = args.s
        client._port = args.p

        return True

    # ******************** MAIN *********************

    @staticmethod
    def main(argv):
        if (not client.parseArguments(argv)):
            client.usage()
            return

        #  Write code here. MAIN CONNECTION CODE

        client.shell()
        print("+++ FINISHED +++")


if __name__ == "__main__":
    client.main([])
