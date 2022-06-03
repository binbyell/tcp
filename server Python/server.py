#서버 코드
import datetime
import threading
import socket
import time

ThisId = "Server"
Spliter = '!'


class Room:
    def __init__(self):
        # 접속한 클라이언트를 담당하는 ChatClient 객체 저장
        self.clients = []
        self.clients_dict = dict()
        self.admins = []
        self.admins_dict = dict()

    # 클라이언트 하나를 Room에 추가
    def addClient(self, c, id_str: str):
        self.clients.append(c)
        self.clients_dict[id_str] = c

    # 클라이언트 하나를 Room에서 삭제
    def delClent(self, c, id_str: str):
        self.clients.remove(c)

    def addAdmin(self, admin, id_str: str):
        self.admins.append(admin)
        self.admins_dict[id_str] = admin

    def delAdmin(self, admin, id_str: str):
        self.admins.remove(admin)
        self.admins = []
        # self.admins_dict

    def sendAllClients(self, msg):
        for key in self.clients_dict.keys():
            self.clients_dict[key].sendMsg(msg)
        # for c in self.clients:
        #     c.sendMsg(msg)

    def sendClients(self, msg, client):
        try:
            self.clients_dict[client].sendMsg(msg)

        except Exception as e:
            print(e)

    def sendAllAdmins(self, msg):
        for key in self.admins_dict.keys():
            self.admins_dict[key].sendMsg(msg)
        # for admin in self.admins:
        #     admin.sendMsg(msg)

    def sendAdmins(self, msg, admin):
        self.admins_dict[admin].sendMsg(msg)


class ChatClient:
    def __init__(self, soc, r):
        self.id = id    #클라이언트 id
        self.soc = soc  #담당 클라이언트와 1:1 통신할 소켓
        self.room = r   #채팅방 객체

    def recvMsg(self):
        while True:

            data = self.soc.recv(1024)
            message = data.decode()
            message = message.strip()
            print(f"recvMsg receive : {message} \ntime:{datetime.datetime.now().strftime('%m/%d/%Y, %H:%M:%S')}")
            splitList = message.split('!')

            nodeId = splitList[0]
            order = splitList[1]
            value = splitList[2]

            """
            order
            - node to control
            /warn
            /emergency
            /emergencyIng
            /warnClear
            /emergencyClear
            /sendT
            """

            sendingMsg = f"{ThisId}{Spliter}/received{Spliter}0\n"
            self.sendMsg(sendingMsg)
            time.sleep(0.1)

            # node to control
            if nodeId[0] == 'N':
                sendingMsg = f"{nodeId}{Spliter}{order}{Spliter}{value}\n"
                print(f"SendMessage To All Admins warn: {sendingMsg}")
                self.room.sendAllAdmins(sendingMsg)

            # control to node
            if order == '/clear':
                sendingMsg = f"{ThisId}{Spliter}{order}{Spliter}{value}\n"
                print(f"SendMessage To All Nodes clear : {sendingMsg}")
                self.room.sendAllClients(sendingMsg)

            if order == '/checkV':
                sendingMsg = f"{ThisId}{Spliter}{order}{Spliter}{value}\n"
                print(f"SendMessage To {value} : {sendingMsg}")
                self.room.sendClients(msg=sendingMsg, client=value)

            if order == '/checkA':
                sendingMsg = f"{ThisId}{Spliter}{order}{Spliter}{value}\n"
                print(f"SendMessage To {value} : {sendingMsg}")
                self.room.sendClients(msg=sendingMsg, client=value)
                # self.room.sendAllClients(sendingMsg)

            if order == '/checkVAll':
                sendingMsg = f"{ThisId}{Spliter}{order}{Spliter}{value}\n"
                print(f"SendMessage To All Nodes checkV : {sendingMsg}")
                self.room.sendAllClients(msg=sendingMsg, client=value)

            if order == '/checkAAll':
                sendingMsg = f"{ThisId}{Spliter}{order}{Spliter}{value}\n"
                print(f"SendMessage To All Nodes checkV : {sendingMsg}")
                self.room.sendAllClients(msg=sendingMsg, client=value)

            if order == '/quit':
                self.room.delAdmin(self)

    # 담당한 클라이언트 1명에게만 메시지 전송
    def sendMsg(self, msg):
        self.soc.sendall(msg.encode(encoding='utf-8'))

    def run(self):
        t = threading.Thread(target=self.recvMsg, args=())
        t.start()


class ServerMain:
    # '192.168.0.11'
    ip = socket.gethostbyname(socket.gethostname())
    print(f"ip: {ip}")
    port = 8092

    def __init__(self):
        self.room = Room()
        self.server_soc = None

    def open(self):
        self.server_soc = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server_soc.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server_soc.bind((ServerMain.ip, ServerMain.port))
        self.server_soc.listen()

    def run(self):
        self.open()
        print('채팅 서버 시작')
        while True:
            c_soc, addr = self.server_soc.accept()
            print(addr)
            # msg = '사용할 id:'
            # c_soc.sendall(msg.encode(encoding='utf-8'))
            # msg = c_soc.recv(1024)
            # id = msg.decode()
            # cc = ChatClient(id, c_soc, self.room)
            cc = ChatClient(c_soc, self.room)
            cc.sendMsg("S1!/checkId!0\n")
            print(f"send : {'S1!/checkId!0'}\n")

            # check ID start
            data = c_soc.recv(1024)
            message = data.decode()
            message = message.strip()
            print(f"receive : {message}")
            splitList = message.split('!')
            nodeId = splitList[0]
            order = splitList[1]

            # admin
            if order == "/sendId" and nodeId[0] == "C":
                self.room.addAdmin(cc, nodeId)
                print(f'ip: {addr} add in Control \n채팅 시작')
                sendingMsg = f"{ThisId}{Spliter}{order}{Spliter}0\n"
                self.room.sendAllAdmins(sendingMsg)
                cc.run()

            # client
            elif order == "/sendId" and nodeId[0] == "N":
                self.room.addClient(cc, nodeId)
                print(f'ip: {addr} add in Node \n채팅 시작')
                cc.run()
            # check Id End

            print(f"clients_dict :\n{self.room.clients_dict}")
            print(f"admins_dict :\n{self.room.admins_dict}")


if __name__ == '__main__':
    server = ServerMain()
    server.run()
