import socket
import threading

def handle_client(client_socket):
    while True:
        client_message = client_socket.recv(1024).decode('utf-8')
        print("客户端消息: " + client_message)
        
        if client_message.upper() == "END":
            print("结束通话...")
            break

def start_server():
    server_ipv6 = socket.getaddrinfo('fe80::f816:3eff:fe87:1247%ens5', 12345, socket.AF_INET6)[0][-1]
    server_socket = socket.socket(socket.AF_INET6, socket.SOCK_STREAM)
    server_socket.bind(server_ipv6)
    server_socket.listen(5)

    print("等待客户端连接...")

    client_socket, client_addr = server_socket.accept()
    print(f"连接已建立：{client_addr}")

    # 接收消息
    client_thread = threading.Thread(target=handle_client, args=(client_socket,))
    client_thread.start()

    # 发送消息
    while True:
        server_message = input("")  # ("你: ")
        client_socket.send(server_message.encode('utf-8'))

        if server_message.upper() == "END":
            print("结束通话...")
            break

    client_socket.close()

try:
    start_server()
except KeyboardInterrupt:
    print("\n服务器关闭.")
