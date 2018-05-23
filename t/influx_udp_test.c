
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>


#define SERVER "127.0.0.1"
#define BUFLEN 512
#define PORT 8089


void
die(char *s)
{
    perror(s);
    exit(1);
}


int main(void)
{
    struct sockaddr_in si_other;
    int                s, slen=sizeof(si_other);
    char               message[BUFLEN];

    if ((s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1) {
        die("socket");
    }

    memset((char *) &si_other, 0, sizeof(si_other));
    si_other.sin_family = AF_INET;
    si_other.sin_port = htons(PORT);

    if (inet_aton(SERVER , &si_other.sin_addr) == 0) {
        fprintf(stderr, "inet_aton() failed\n");
        exit(1);
    }

    snprintf(message, sizeof(message),
            "weather,location=us-midwest-a temperature=1200 1465839850700400200\n"
            "weather,location=us-midwest-b temperature=120 1465839850900400200\n"
            );

    //send the message
    if (sendto(s, message, strlen(message), 0,
                (struct sockaddr *) &si_other, slen) == -1)
    {
        die("sendto()");
    }

    fprintf(stdout, "Message \"%s\" was sent.\n", message);

    close(s);

    return EXIT_SUCCESS;
}

