#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>

#define PORT 5555  // Port d'écoute de l'agent
#define BUFFER_SIZE 1024

// Fonction pour calculer le préfixe à partir du masque réseau
int calculer_prefixe(struct sockaddr *netmask) {
    int prefixe = 0;
    unsigned char *octets;
    int taille;

    if (netmask->sa_family == AF_INET) {
        octets = (unsigned char *)&(((struct sockaddr_in *)netmask)->sin_addr);
        taille = 4; // IPv4
    } else if (netmask->sa_family == AF_INET6) {
        octets = (unsigned char *)&(((struct sockaddr_in6 *)netmask)->sin6_addr);
        taille = 16; // IPv6
    } else {
        return -1; // Type non supporté
    }

    for (int i = 0; i < taille; i++) {
        while (octets[i]) {
            prefixe += octets[i] & 1;
            octets[i] >>= 1;
        }
    }

    return prefixe;
}

// Fonction pour récupérer les informations des interfaces
void get_network_info(char *buffer, size_t size) {
    struct ifaddrs *interfaces, *ifa;
    char adresse[INET6_ADDRSTRLEN];
    int famille;

    if (getifaddrs(&interfaces) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    buffer[0] = '\0';  // Initialisation du buffer

    for (ifa = interfaces; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL) continue;

        famille = ifa->ifa_addr->sa_family;
        if (famille == AF_INET || famille == AF_INET6) {
            snprintf(buffer + strlen(buffer), size - strlen(buffer), "%s : ", ifa->ifa_name);

            if (famille == AF_INET) {
                struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
                inet_ntop(AF_INET, &addr_in->sin_addr, adresse, sizeof(adresse));
                snprintf(buffer + strlen(buffer), size - strlen(buffer), "IPv4 : %s/", adresse);
            } else if (famille == AF_INET6) {
                struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)ifa->ifa_addr;
                inet_ntop(AF_INET6, &addr_in6->sin6_addr, adresse, sizeof(adresse));
                snprintf(buffer + strlen(buffer), size - strlen(buffer), "IPv6 : %s/", adresse);
            }

            if (ifa->ifa_netmask) {
                int prefixe = calculer_prefixe(ifa->ifa_netmask);
                if (prefixe >= 0) {
                    snprintf(buffer + strlen(buffer), size - strlen(buffer), "%d\n", prefixe);
                } else {
                    snprintf(buffer + strlen(buffer), size - strlen(buffer), "inconnu\n");
                }
            } else {
                snprintf(buffer + strlen(buffer), size - strlen(buffer), "inconnu\n");
            }
        }
    }

    freeifaddrs(interfaces);
}

// Fonction principale du serveur
int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[BUFFER_SIZE];

    // Création du socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Liaison du socket à l'adresse et au port
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    // Écoute des connexions entrantes
    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Agent en écoute sur le port %d...\n", PORT);

    while (1) {
        // Acceptation d'un client
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        printf("Client connecté\n");

        // Récupération des informations et envoi au client
        get_network_info(buffer, sizeof(buffer));
        send(client_fd, buffer, strlen(buffer), 0);

        close(client_fd);
    }

    close(server_fd);
    return 0;
}

