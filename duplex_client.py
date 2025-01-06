import socket
import threading

def handle_server(server_socket):
    while True:
        server_message = server_socket.recv(1024).decode('utf-8')
        print("服务器消息: " + server_message)
        
        if server_message.upper() == "END":
            print("结束通话...")
            break

def start_client():
    client_ipv6 = socket.getaddrinfo('fe80::f816:3eff:fe87:1247%ens5', 12345, socket.AF_INET6)[0][-1]
    client_socket = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
    client_socket.connect(client_ipv6)

    # 接受消息
    server_thread = threading.Thread(target=handle_server, args=(client_socket,))
    server_thread.start()

    # 发送消息
    while True:
        client_message = input("")  # ("你: ")
        client_socket.send(client_message.encode('utf-8'))

        if client_message.upper() == "END":
            print("结束通话...")
            break

    client_socket.close()

try:
    start_client()
except KeyboardInterrupt:
    print("\n客户端关闭.")
