#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 67 
#define MAX_BUFFER_SIZE 1024
#define IP_POOL_SIZE 10

char *ip_pool[IP_POOL_SIZE] = {"192.168.1.10", "192.168.1.11", "192.168.1.12"};
int ip_allocated[IP_POOL_SIZE] = {0, 0, 0}; // Array para verificar IPs asignadas

void handle_dhcpdiscover(int socket_client, struct sockaddr_in *client_addr, socklen_t addr_len) {
    char *assigned_ip = NULL;
    for (int i = 0; i < IP_POOL_SIZE; i++) {
        if (ip_allocated[i] == 0) { 
            assigned_ip = ip_pool[i]; 
            ip_allocated[i] = 1; 
            break; 
        }
    }

    printf("Se ofreció la IP %s al cliente.\n", assigned_ip ? assigned_ip : "No hay IPs disponibles");
    if (assigned_ip != NULL) {
        sendto(socket_client, assigned_ip, strlen(assigned_ip), 0, (struct sockaddr *)client_addr, addr_len);
    } else {
        char *no_ip_message = "No hay IPs disponibles.";
        sendto(socket_client, no_ip_message, strlen(no_ip_message), 0, (struct sockaddr *)client_addr, addr_len);
    }
}

void handle_client(int socket_client) {
    char buffer[MAX_BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    int n;

    while (1) {
        memset(buffer, 0, sizeof(buffer));
        n = recvfrom(socket_client, buffer, sizeof(buffer), 0, (struct sockaddr *)&client_addr, &addr_len);

        if (n < 0) {
            perror("Error en la comunicación");
            continue;
        }

        printf("Mensaje recibido: %s\n", buffer);

        if (strcmp(buffer, "DHCPDISCOVER") == 0) {
            handle_dhcpdiscover(socket_client, &client_addr, addr_len);
        } else {
            printf("Petición no reconocida: %s\n", buffer);
        }
    }
}

int main() {
    int socket_client;
    struct sockaddr_in server_addr;

    socket_client = socket(AF_INET, SOCK_DGRAM, 0);  // Usar SOCK_DGRAM para UDP
    if (socket_client < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // Escuchar en todas las interfaces
    server_addr.sin_port = htons(PORT);

    if (bind(socket_client, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(socket_client);
        exit(EXIT_FAILURE);
    }

    printf("Servidor DHCP en ejecución en el puerto %i ...\n", PORT);

    handle_client(socket_client); // Manejar clientes

    close(socket_client);
    return 0;
}
