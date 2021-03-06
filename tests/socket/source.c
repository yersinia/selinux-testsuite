#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>		/* for inet_aton() */
#include <sys/poll.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

static const char *hostname = "127.0.0.1";
static const u_short port = 4000;
static const int sendbytes = 100;	/* Send this much data */

int
main(int argc, char **argv)
{
	char data[sendbytes];
	struct sockaddr_in saddr;
	int protocol;
	int sock;
	int result;
	struct pollfd fdset;

	memset(data, 'X', sendbytes);

	if (argc < 2) {
		printf("%s <protocol>\n", argv[0]);
		return -1;
	}

	if (strcmp(argv[1], "stream") == 0)
		protocol = SOCK_STREAM;
	else
		protocol = SOCK_DGRAM;

	bzero(&saddr, sizeof (struct sockaddr_in));

	if (!inet_aton(hostname, &saddr.sin_addr)) {
		printf("error converting address\n");
		return -1;
	}

	saddr.sin_family = AF_INET;
	saddr.sin_port = htons(port);

	if ((sock = socket(saddr.sin_family, protocol, 0)) < 0) {
		perror("socket");
		return -1;
	}

	if ((result = fcntl(sock, F_SETFL, O_NONBLOCK))) {
		perror("fcntl: ");
		return -1;
	}
	result = connect(sock, (struct sockaddr *) &saddr, sizeof (saddr));
	if (result < 0) {
		if (errno == EINPROGRESS) {
			fdset.fd = sock;
			fdset.events = POLLIN | POLLOUT;
			result = poll(&fdset, 1, 1000);
			if (result < 0) {
				perror("poll: ");
				return -1;
			} else if (result == 0) {
				/* 
				 * connect is taking too long, 
				 * it's probably not going to work 
				 */
				printf("poll: no events are ready\n");
				return -1;
			} else if (result == 1 && fdset.revents & POLLHUP) {
				printf("poll: connection closed\n");
				return -1;
			}
		} else {
			perror("connect");
			close(sock);
			return -1;
		}
	}

	result = write(sock, data, sendbytes);
	if (result != sendbytes) {
		printf("Bad write %d of %d\n", result, sendbytes);
		return -1;
	}
	printf("sent %5d bytes\n", result);
	shutdown(sock, 2);
	close(sock);

	return (0);
}
