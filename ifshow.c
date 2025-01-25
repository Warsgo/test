#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <ifaddrs.h>

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
void afficher_infos_interface(const char *nom_interface) {
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
            printf("%s : ", ifa->ifa_name);

            if (famille == AF_INET) { // Adresse IPv4
                struct sockaddr_in *addr_in = (struct sockaddr_in *)ifa->ifa_addr;
                inet_ntop(AF_INET, &addr_in->sin_addr, adresse, sizeof(adresse));
                printf("IPv4 : %s/", adresse);
            } else if (famille == AF_INET6) { // Adresse IPv6
                struct sockaddr_in6 *addr_in6 = (struct sockaddr_in6 *)ifa->ifa_addr;
                inet_ntop(AF_INET6, &addr_in6->sin6_addr, adresse, sizeof(adresse));
                printf("IPv6 : %s/", adresse);
            }

            // Ajoute le préfixe
            if (ifa->ifa_netmask) {
                int prefixe = calculer_prefixe(ifa->ifa_netmask);
                if (prefixe >= 0) {
                    printf("%d\n", prefixe);
                } else {
                    printf("inconnu\n");
                }
            } else {
                printf("inconnu\n");
            }
        }
    }

    // Libère la mémoire allouée pour les interfaces
    freeifaddrs(interfaces);
}

// Fonction pour afficher toutes les interfaces
void afficher_toutes_les_interfaces() {
    afficher_infos_interface(NULL);
}

// Fonction principale
int main(int argc, char *argv[]) {
    // Vérifie les arguments fournis au programme
    if (argc < 2) {
        fprintf(stderr, "Utilisation : %s -i nom_interface | -a\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Si l'option -a est spécifiée, afficher toutes les interfaces
    if (strcmp(argv[1], "-a") == 0) {
        afficher_toutes_les_interfaces();
    } 
    // Si l'option -i est spécifiée avec un nom d'interface
    else if (strcmp(argv[1], "-i") == 0 && argc == 3) {
        afficher_infos_interface(argv[2]);
    } 
    // Si les arguments sont invalides
    else {
        fprintf(stderr, "Arguments invalides\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}