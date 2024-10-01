import socket

# Definir la IP y el puerto del servidor
SERVER_IP = "127.0.0.1"
SERVER_PORT = 67

def dhcp_discover():
    # Crear un socket de cliente
    client_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)  # Cambiar a SOCK_DGRAM

    # Enviar solicitud DHCP
    message = "DHCPDISCOVER"
    client_socket.sendto(message.encode(), (SERVER_IP, SERVER_PORT))  # Usar sendto para UDP

    # Recibir respuesta del servidor
    response, _ = client_socket.recvfrom(1024)  # Recibir la respuesta
    print(f"IP asignada por el servidor: {response.decode()}")

    # Cerrar conexión
    client_socket.close()

if __name__ == "__main__":
    dhcp_discover()
