#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <set>
#include <string>

#define PORT 12345
#define BUFFER_SIZE 1024
#define BROADCAST_IP "255.255.255.255"  // Adresse de diffusion globale

void discover_neighbors(int hops) {
    int sock;
    struct sockaddr_in broadcast_addr;
    char buffer[BUFFER_SIZE];
    char recv_buffer[BUFFER_SIZE] = {0};
    std::set<std::string> unique_hosts;

    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    int broadcast = 1;
    if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &broadcast, sizeof(broadcast)) < 0) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_port = htons(PORT);
    broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Préparer le message de découverte avec le nombre de sauts
    snprintf(buffer, BUFFER_SIZE, "DISCOVER %d", hops);

    // Envoyer la requête de découverte
    if (sendto(sock, buffer, strlen(buffer), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
        perror("sendto");
        exit(EXIT_FAILURE);
    }

    printf("Requête de découverte envoyée avec %d sauts\n", hops);

    // Recevoir les réponses
    struct sockaddr_in from_addr;
    socklen_t from_len = sizeof(from_addr);
    while (1) {
        int len = recvfrom(sock, recv_buffer, BUFFER_SIZE, 0, (struct sockaddr *)&from_addr, &from_len);
        if (len < 0) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        recv_buffer[len] = '\0';
        std::string hostname(recv_buffer);

        // Ajouter le nom de la machine à l'ensemble pour éviter les doublons
        if (unique_hosts.find(hostname) == unique_hosts.end()) {
            unique_hosts.insert(hostname);
            printf("Machine voisine: %s\n", hostname.c_str());
        }
    }

    close(sock);
}

int main(int argc, char *argv[]) {
    int hops = 1;  // Valeur par défaut pour les sauts

    // Vérifier les arguments de la ligne de commande
    if (argc == 3 && strcmp(argv[1], "-hop") == 0) {
        hops = atoi(argv[2]);
    }

    discover_neighbors(hops);
    return 0;
}
Mise à jour de agent.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUFFER_SIZE 1024
#define BROADCAST_IP "255.255.255.255"  // Adresse de diffusion globale

int main() {
    int server_fd;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_SIZE] = {0};
    char hostname[BUFFER_SIZE];

    gethostname(hostname, BUFFER_SIZE);

    if ((server_fd = socket(AF_INET, SOCK_DGRAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    printf("Agent en écoute sur le port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_addr_len = sizeof(client_addr);
        int len = recvfrom(server_fd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr, &client_addr_len);
        if (len < 0) {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        buffer[len] = '\0';

        // Extraire le nombre de sauts de la requête
        int hops;
        sscanf(buffer, "DISCOVER %d", &hops);

        // Envoyer le nom de la machine
        sendto(server_fd, hostname, strlen(hostname), 0, (struct sockaddr *)&client_addr, client_addr_len);
       ("Nom de la machine envoyé: %s\n", hostname);

        // Propager la requête si le nombre de sauts est supérieur à 1
        if (hops > 1) {
            struct sockaddr_in broadcast_addr;
            broadcast_addr.sin_family = AF_INET;
            broadcast_addr.sin_port = htons(PORT);
            broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

            // Préparer le message de découverte avec le nombre de sauts décrémenté
            snprintf(buffer, BUFFER_SIZE, "DISCOVER %d", hops - 1);

            // Envoyer la requête de découverte
            if (sendto(server_fd, buffer, strlen(buffer), 0, (struct sockaddr *)&broadcast_addr, sizeof(broadcast_addr)) < 0) {
                perror("sendto");
                exit(EXIT_FAILURE);
            }

            printf("Requête de découverte propagée avec %d sauts\n", hops - 1);
        }
    }

    return 0;
}
