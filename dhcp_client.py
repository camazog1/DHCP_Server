import socket

# Definir la IP y el puerto del servidor
SERVER_IP = "127.0.0.1"
SERVER_PORT = 67

def dhcp_discover():
    # Crear un socket de cliente
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    client_socket.connect((SERVER_IP, SERVER_PORT))

    # Enviar solicitud DHCP
    message = "DHCPDISCOVER"
    client_socket.send(message.encode())

    # Recibir respuesta del servidor
    response = client_socket.recv(1024).decode()
    print(f"IP asignada por el servidor: {response}")

    # Cerrar conexión
    client_socket.close()

if __name__ == "__main__":
    dhcp_discover()
