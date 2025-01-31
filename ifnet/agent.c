#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>
#include <unistd.h>

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

// Fonction pour afficher les informations de l'interface spécifiée
void afficher_infos_interface(const char *nom_interface, char *buffer, size_t size) {
    struct ifaddrs *interfaces, *ifa;
    char adresse[INET6_ADDRSTRLEN];
    int famille;

    // Récupère la liste des interfaces réseau
    if (getifaddrs(&interfaces) == -1) {
        perror("getifaddrs");
        exit(EXIT_FAILURE);
    }

    // Parcours toutes les interfaces
    for (ifa = interfaces; ifa != NULL; ifa = ifa->ifa_next) {
        // Si un nom d'interface est spécifié, on ne traite que celle-ci
        if (nom_interface && strcmp(nom_interface, ifa->ifa_name) != 0) {
            continue;
        }

        if (ifa->ifa_addr == NULL) {
            continue;
        }

        famille = ifa->ifa_addr->sa_family;

        // Vérifie si c'est une adresse IPv4 ou IPv6
        if (famille == AF_INET || famille == AF_INET6) {
            snprintf(buffer + strlen(buffer), size - strlen(buffer), "%s : ", ifa->ifa_name);

            if (famille == AF_INET) { // Adresse IPv4
                struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
                inet_ntop(AF_INET, &addr_in->sin_addr, adresse, sizeof(adresse));
                snprintf(buffer + strlen(buffer), size - strlen(buffer), "IPv4 : %s/", adresse);
            } else if (famille == AF_INET6) { // Adresse IPv6
                struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)ifa->ifa_addr;
                inet_ntop(AF_INET6, &addr_in6->sin6_addr, adresse, sizeof(adresse));
                snprintf(buffer + strlen(buffer), size - strlen(buffer), "IPv6 : %s/", adresse);
            }

            // Ajoute le préfixe
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

    // Libère la mémoire allouée pour les interfaces
    freeifaddrs(interfaces);
}

// Fonction pour afficher toutes les interfaces
void afficher_toutes_les_interfaces(char *buffer, size_t size) {
    afficher_infos_interface(NULL, buffer, size);
}

// Fonction principale
int main() {
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char buffer[1024] = {0}; // Initialise le tampon

    // Création du socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(5555);

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

    printf("Agent en écoute sur le port 5555...\n");

    while (1) {
        // Acceptation d'un client
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if (client_fd == -1) {
            perror("accept");
            continue;
        }

        printf("Client connecté\n");

        // Réception de la requête du client
        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer) - 1, 0);
        if (bytes_received <= 0) {
            if (bytes_received < 0) perror("recv");
            close(client_fd);
            continue;
        }
        buffer[bytes_received] = '\0'; // Assurer la terminaison de la chaîne
        
        // Réinitialise le buffer pour éviter l'accumulation des données
        char response[1024] = {0}; 

        // Traitement de la requête
        if (strcmp(buffer, "ALL") == 0) { // Pour la commande -a
            afficher_toutes_les_interfaces(response, sizeof(response));
        } else if (strncmp(buffer, "IFNAME ", 7) == 0) { // Pour la commande -i
            afficher_infos_interface(buffer + 7, response, sizeof(response));
        }

        // Envoi de la réponse au client
        send(client_fd, response, strlen(response), 0);

        close(client_fd);
    }

    close(server_fd);
    return 0;
}

