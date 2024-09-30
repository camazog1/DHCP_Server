#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 67  // Puerto estándar DHCP
#define MAX_BUFFER_SIZE 1024
#define IP_POOL_SIZE 10

char *ip_pool[IP_POOL_SIZE] = {"192.168.1.10", "192.168.1.11", "192.168.1.12"}; // Pool de direcciones IP

void *handle_dhcp_request(void *client_socket) {
    int new_socket = *((int *)client_socket);
    free(client_socket);

    char buffer[MAX_BUFFER_SIZE];
    read(new_socket, buffer, sizeof(buffer));
    printf("Mensaje recibido: %s\n", buffer);

    // Lógica para responder al cliente con una IP del pool
    char *response = "192.168.1.10";  // Solo asignando una IP fija por ahora
    write(new_socket, response, strlen(response));
    close(new_socket);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);

    // Crear socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Error en la creación del socket");
        exit(EXIT_FAILURE);
    }

    // Configuración del socket
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
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
        pthread_t thread_id;
        int *client_socket = malloc(sizeof(int));
        *client_socket = new_socket;
        pthread_create(&thread_id, NULL, handle_dhcp_request, client_socket);
    }

    return 0;
}
