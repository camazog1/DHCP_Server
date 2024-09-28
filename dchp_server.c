#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>
#pragma comment(lib, "Ws2_32.lib")  // Enlazar la librería Winsock
#else
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#endif

#define PORT 67  // Puerto estándar DHCP
#define MAX_BUFFER_SIZE 1024
#define IP_POOL_SIZE 10

char *ip_pool[IP_POOL_SIZE] = {"192.168.1.10", "192.168.1.11", "192.168.1.12"}; // Pool de direcciones IP

#ifdef _WIN32
DWORD WINAPI handle_dhcp_request(LPVOID client_socket) {
#else
void *handle_dhcp_request(void *client_socket) {
#endif
    int new_socket = *((int *)client_socket);
    free(client_socket);

    char buffer[MAX_BUFFER_SIZE];
    recv(new_socket, buffer, sizeof(buffer), 0);
    printf("Mensaje recibido: %s\n", buffer);

    // Lógica para responder al cliente con una IP del pool
    char *response = "192.168.1.10";  // Solo asignando una IP fija por ahora
    send(new_socket, response, strlen(response), 0);
#ifdef _WIN32
    closesocket(new_socket);
#else
    close(new_socket);
#endif
    return 0;
}

int main() {
    int server_fd;
#ifdef _WIN32
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        printf("Error al inicializar Winsock.\n");
        return 1;
    }
    SOCKET new_socket;
#else
    int new_socket;
#endif

    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error en la creación del socket");
        exit(EXIT_FAILURE);
    }

#ifdef _WIN32
    // Configuración del socket en Windows
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt))) {
#else
    // Configuración del socket en sistemas Unix/Linux
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
#endif
        perror("Error en configuración de socket");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Asignar puerto al socket
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Error en bind");
        exit(EXIT_FAILURE);
    }

    // Escuchar conexiones
    if (listen(server_fd, 3) < 0) {
        perror("Error en listen");
        exit(EXIT_FAILURE);
    }
    printf("Servidor DHCP en escucha en el puerto %d\n", PORT);

    // Aceptar conexiones entrantes y crear hilos para cada cliente
    while ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) >= 0) {
#ifdef _WIN32
        // Crear hilo en Windows
        HANDLE thread_id;
        int *client_socket = (int *)malloc(sizeof(int));
        *client_socket = new_socket;
        thread_id = CreateThread(NULL, 0, handle_dhcp_request, client_socket, 0, NULL);
        if (thread_id == NULL) {
            printf("Error al crear el hilo\n");
        }
#else
        // Crear hilo en Linux/Unix
        pthread_t thread_id;
        int *client_socket = malloc(sizeof(int));
        *client_socket = new_socket;
        pthread_create(&thread_id, NULL, handle_dhcp_request, client_socket);
#endif
    }

#ifdef _WIN32
    WSACleanup();  // Limpiar Winsock
#endif

    return 0;
}
