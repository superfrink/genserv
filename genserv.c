/* Chad C. D. Clark  < frink _at_ superfrink _dot_ net >
 *
 * file: genserv.c
 * purpose:
 *   A general purpose concurrent server.  This program handles the network
 *   interface.  Each request is serviced by accepting an optional password
 *   and then executing a specified program.
 *
 * $Id: genserv.c,v 1.4 2003/08/08 05:48:53 frink Exp $
 *
 * TODO: add a basic password authentiction.
 *
 */

#include<stdio.h>
#include<string.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<linux/in.h>
#include<unistd.h>
#include<signal.h>

/****************************************************************************/
/* Debugging strategy comes from Alessandro Rubini's "Linux Device Drivers. */

/* #define DEBUG_ON */

#undef PDEBUG
#ifdef DEBUG_ON
#define PDEBUG(fmt, args...)    printf (fmt, ## args)

#else
#define PDEBUG(fmt, args...)    /* do nothing */

#endif      // #define DEBUG_ON

/*************************************************************************/


int main(int argc, char **argv) {

	int	port_num = 12321;	// port to listen to
	int	sockfd;		// listening file descriptor
	int	reqfd;		// request file descriptor
	int	socklen;	// size of sockadd_in struct
	struct	sockaddr_in servaddr;
	int	one = 1;
	char	msg[] = "Hi from server land!\n";
	char	*exec_cmd;	// program to be executed.
	int	MAX_ARGS = 64;		// max args possible.
	char	*exec_args[MAX_ARGS];	// array of arguments to execvp()
	int	i = 0;		// loop variable


	if (argc < 3) {

		printf("usage:\n");
		printf("\t %s port_num program_name args\n", argv[0]);
		printf("where:\n");
		printf("  \"port_num\" is a valid TCP port number.\n");
		printf("  \"program_name\" is a program to be run when a ");
		printf("client connects.\n");
		printf("  \"args\" is an optional list of arguments to ");
		printf("the program to be executed.\n");
		printf("\neg: %s 12345 /usr/bin/uptime \n");
		printf("    will listen on port 12345 and print the uptime ");
		printf("string then exit.\n");
		printf("\neg: %s 12345 /usr/bin/uptime -V\n");
		printf("    will listen on port 12345 and print the uptime ");
		printf("version then exit.\n");
		exit(-1);
	}

	port_num = atoi(argv[1]);

	exec_cmd = argv[2];

	for(i = 0; i < argc; i++) {
		exec_args[i] = argv[i+2];
	}
	exec_args[argc + 2] = 0;

	printf("\n%s: Using port %d.\n", argv[0], port_num);
	printf("%s: Will execute: ", argv[0]);
	i = 0;
	while (exec_args[i] != 0) {
		printf("%s ", exec_args[i]);
		i++;
	}
	printf("\n");

	if ( (sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {

		printf("\n%s: fatal error calling socket()\n", argv[0]);
		exit(-2);
	}

	setsockopt(sockfd, SOL_SOCKET, 0, &one, sizeof(one));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(port_num);
	socklen = sizeof(servaddr);
	if( (bind(sockfd, (struct sockaddr *) &servaddr, socklen)) < 0) {

		printf("\n%s: fatal error calling bind()\n", argv[0]);
		exit(-3);
	}

	PDEBUG("\n%s: bound\n", argv[0]);

	if((listen(sockfd, 5)) < 0) {

		printf("\n%s: fatal error calling listen()\n", argv[0]);
		exit(-4);
	}

	PDEBUG("\n%s: listening\n", argv[0]);

	/* I don't care about when the children exit and don't want
	 * zombies building up so ignore sig_child.
	 */
	if(signal(SIGCHLD, SIG_IGN) == SIG_ERR) {

		printf("\n%s: Unable to ignore SIGCHLD. ", argv[0]);
		printf("Still running but zombies could overrun the system!\n");
	}

	while( (reqfd = (accept(sockfd,
			(struct sockaddr *) &servaddr,
			&socklen))
		) >= 0) {

		PDEBUG("\n%s: request arrived (fd=%d)\n", argv[0], reqfd);

		if( fork() == 0) {
			// we're in the child process

			// the child doesn't keep listening to the network.
			close(sockfd);

			// move the network file desc to stdin/out/err 
			dup2(reqfd, 0);
			dup2(reqfd, 1);
			dup2(reqfd, 2);

			// serve the request.
			PDEBUG("%s\n", exec_cmd);

			if ( execvp(exec_cmd, exec_args) ) {
				printf("%s: ERROR running %s\n",
					argv[0],
					exec_cmd
				);
				perror(0);
			}

			// should never get here.
			close(reqfd);
			exit(0);

		} else {

			// still the parent proccess, keep listening to the net.

			// the parent doesn't serve the request
			close(reqfd);
		}
	}


	return(0);
}

