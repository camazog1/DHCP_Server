#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <time.h> 

#define LEASE_TIME 3  // 300 segundos = 5 minutos
#define PORT 67 
#define MAX_BUFFER_SIZE 1024
#define IP_POOL_SIZE 10
#define MAX_THREADS 4 
#define DNS "8.8.8.8"
#define NETMASK "255.255.255.0"


char GATEWAY[NI_MAXHOST];

char *ip_pool[IP_POOL_SIZE];
int ip_allocated[IP_POOL_SIZE] = {0};
time_t ip_lease_time[IP_POOL_SIZE];
int active_threads = 0;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

void check_ip_leases() {
    time_t current_time = time(NULL); 
    for (int i = 0; i < IP_POOL_SIZE; i++) {
        if (ip_allocated[i] && difftime(current_time, ip_lease_time[i]) >= LEASE_TIME) {
            ip_allocated[i] = 0; 
            printf("IP %s ha sido liberada por expiración del tiempo de vida.\n", ip_pool[i]);
        }
    }
}


void get_server_ip(char *GATEWAY, size_t size) {
    struct ifaddrs *ifaddr, *ifa;
    int family;
    char host[NI_MAXHOST];

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    // Recorre todas las interfaces de red
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        family = ifa->ifa_addr->sa_family;

        // Solo IPv4 (AF_INET)
        if (family == AF_INET) {
            if (getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0) {
                if (strcmp(ifa->ifa_name, "lo") != 0) {
                    strncpy(GATEWAY, host, size);
                    break;
                }
            }
        }
    }

    freeifaddrs(ifaddr);
}

void generate_ip_pool(char *ip_pool[], const char *start_ip, const char *end_ip) {
    int start[4], end[4];
    sscanf(start_ip, "%d.%d.%d.%d", &start[0], &start[1], &start[2], &start[3]);
    sscanf(end_ip, "%d.%d.%d.%d", &end[0], &end[1], &end[2], &end[3]);

    int idx = 0; 
    for (int i = start[2]; i <= end[2]; i++) {
        for (int j = start[3]; j <= end[3]; j++) {
            if (idx >= IP_POOL_SIZE) return;
            snprintf(ip_pool[idx], 16, "%d.%d.%d.%d", start[0], start[1], i, j);
            idx++;
        }
    }
}

void handle_dhcprelease(int socket_client, struct sockaddr_in *client_addr, socklen_t addr_len, const char *released_ip) {
    for (int i = 0; i < IP_POOL_SIZE; i++) {
        if (strcmp(ip_pool[i], released_ip) == 0) {
            ip_allocated[i] = 0; 
            printf("IP liberada: %s\n", ip_pool[i]);

            char mensaje[256]; 
            sprintf(mensaje, "IP %s liberada correctamente", ip_pool[i]);
            sendto(socket_client, mensaje, strlen(mensaje), 0, (struct sockaddr *)client_addr, addr_len);
            return;
        }
    }
    printf("IP no encontrada en el pool: %s\n", released_ip);
    char mensaje[256]; 
    sprintf(mensaje, "Error: IP no encontrada");
    sendto(socket_client, mensaje, strlen(mensaje), 0, (struct sockaddr *)client_addr, addr_len);
}

void dhcp_acknowledge(int socket_client, struct sockaddr_in *client_addr, socklen_t addr_len, const char *ip_address) {
    if (ip_address != NULL && strlen(ip_address) > 0) {
        // Si la dirección IP es válida, envía "OK"
        char mensaje[256];
        sprintf(mensaje, "OK: IP %s aceptada", ip_address);
        sendto(socket_client, mensaje, strlen(mensaje), 0, (struct sockaddr *)client_addr, addr_len);
        printf("IP %s aceptada.\n", ip_address);
    } else {
        // Si la dirección IP es inválida, envía "Reintentar"
        char *reintentar_message = "Reintentar: IP inválida";
        sendto(socket_client, reintentar_message, strlen(reintentar_message), 0, (struct sockaddr *)client_addr, addr_len);
        printf("IP inválida. Se solicitó reintentar.\n");
    }
}

void handle_dhcprequest(int socket_client, struct sockaddr_in *client_addr, socklen_t addr_len, const char *requested_ip) {
    int found = 0;
    
    for (int i = 0; i < IP_POOL_SIZE; i++) {
        if (strcmp(ip_pool[i], requested_ip) == 0 && ip_allocated[i] == 0) {
            ip_allocated[i] = 1;  // Asigna la IP
            ip_lease_time[i] = time(NULL);  // Guarda el tiempo de asignación
            found = 1;
            break;
        }
    }

    if (found) {
        dhcp_acknowledge(socket_client, client_addr, addr_len, requested_ip);
    } else {
        dhcp_acknowledge(socket_client, client_addr, addr_len, NULL);  
    }
}

void handle_dhcpdiscover(int socket_client, struct sockaddr_in *client_addr, socklen_t addr_len) {
    char *assigned_ip = NULL;
    time_t current_time = time(NULL);  

    for (int i = 0; i < IP_POOL_SIZE; i++) {
        if (ip_allocated[i] == 0) {
            assigned_ip = ip_pool[i]; 
            ip_lease_time[i] = current_time; 
            break; 
        }
    }

    printf("Se ofreció la IP %s al cliente.\n", assigned_ip ? assigned_ip : "No hay IPs disponibles");
    if (assigned_ip != NULL) {
        char mensaje[256]; 
        sprintf(mensaje, "IP: %s\nMascara: %s\nDNS: %s\nGateway: %s\nTiempo de arrendamiento: %d segundos", assigned_ip, NETMASK, DNS, GATEWAY, LEASE_TIME);
        sendto(socket_client, mensaje, strlen(mensaje), 0, (struct sockaddr *)client_addr, addr_len);
    } else {
        char *no_ip_message = "No hay IPs disponibles.";
        sendto(socket_client, no_ip_message, strlen(no_ip_message), 0, (struct sockaddr *)client_addr, addr_len);
    }
}

void *handle_client(void *arg) {
    int socket_client = *(int *)arg;
    free(arg);  // Liberar memoria asignada para el socket del cliente

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
        } 
        else if (strstr(buffer, "DHCPREQUEST") != NULL) {
            char *released_ip = strtok(buffer + strlen("DHCPREQUEST: "), " ");
            if (released_ip != NULL) {
                handle_dhcprequest(socket_client, &client_addr, addr_len, released_ip);
            }
        } 
        else if (strstr(buffer, "DHCPRELEASE") != NULL) {
            char *released_ip = strtok(buffer + strlen("DHCPRELEASE: "), " ");
            if (released_ip != NULL) {
                handle_dhcprelease(socket_client, &client_addr, addr_len, released_ip);
            }
        }
        else {
            printf("Petición no reconocida: %s\n", buffer);
        }
    }

    close(socket_client);

    pthread_mutex_lock(&mutex);
    active_threads--;  // Disminuir el contador de hilos activos
    pthread_mutex_unlock(&mutex);

    return NULL;
}

void init_gateway() {
    get_server_ip(GATEWAY, sizeof(GATEWAY));
}

int main() {
    for (int i = 0; i < IP_POOL_SIZE; i++) {
        ip_pool[i] = malloc(16 * sizeof(char));
    }

    generate_ip_pool(ip_pool, "192.168.0.1", "192.168.0.10");
    
    init_gateway();
    printf("Server IP (Gateway): %s\n", GATEWAY);

    int socket_server;
    struct sockaddr_in server_addr;

    socket_server = socket(AF_INET, SOCK_DGRAM, 0);  
    if (socket_server < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    if (bind(socket_server, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(socket_server);
        exit(EXIT_FAILURE);
    }

    printf("Servidor DHCP en ejecución en el puerto %i ...\n", PORT);

    while (1) {
        check_ip_leases();
        pthread_mutex_lock(&mutex);
        if (active_threads < MAX_THREADS) {  // Limitar a 4 hilos
            pthread_mutex_unlock(&mutex);

            int *client_socket = malloc(sizeof(int));
            *client_socket = socket_server; 

            pthread_t thread;
            if (pthread_create(&thread, NULL, handle_client, client_socket) != 0) {
                perror("Error creando el hilo");
                free(client_socket);
            } else {
                pthread_detach(thread);  // No necesitamos esperar a que termine el hilo
                pthread_mutex_lock(&mutex);
                active_threads++;  // Incrementar el contador de hilos activos
                pthread_mutex_unlock(&mutex);
            }
        } else {
            pthread_mutex_unlock(&mutex);
            sleep(1); // Espera un momento antes de volver a intentar crear hilos
        }
    }

    close(socket_server);
    return 0;
}
