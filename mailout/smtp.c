/* smpt.c 03/Nov/2000 (c) Jeremy C. Reed */

#include "includes.h"
#include "config.h"

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

int makeconnection(hname)
	char *hname;
{
  int s;
  struct sockaddr_in server;
  struct hostent *hent;

  s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
  /* maybe should use PF_INET instead of AF_INET */
  /* later allow AF_INET6 */
  if (s < 0) {
    perror ("mailout: socket"); /* later do better logging */
    exit(1);
  }

  hent = (struct hostent *)gethostbyname(hname);
  if (hent == NULL)
    server.sin_addr.s_addr = inet_addr(hname);
  else 
    memcpy(&server.sin_addr.s_addr, hent->h_addr, hent->h_length); 
/* bcopy(hent->h_addr_list[0], &server.sin_addr, hent->h_length); */

  server.sin_family = AF_INET;
  server.sin_port = htons(25); /* later allow different port */

  if (connect (s, (struct sockaddr *)&server, sizeof(server))) {
    perror ("mailout: connect");
    exit(1);
  }

  /*  (void) setsockopt(sock, SOL_IP, IP_TOS, (char *)&on, sizeof(on)); */

  return s;

}

int communicate (s)
  int s;
{
  char buf[1024];
  int loop;

  memset(buf, '\0', 1024);

  /* assume mail server will send banner message */
  if (read(s, buf, 1024) < 1) {
    perror ("mailout: read");
    exit(1);
  }

#ifdef DEBUG
  printf ("%s", buf);
#endif

/* should be 220 */

  write (s, "HELO test\r\n", 11);

  memset(buf, '\0', 1024);

/* should reply with 250 */
  if (read(s, buf, 1024) < 1) {
    perror ("mailout: read");
    exit(1);
  }

#ifdef DEBUG
  printf ("%s", buf);
#endif

  memset(buf, '\0', 1024);

  write (s, "quit\r\n", 6);

/* should reply with 221 */
  if (read(s, buf, 1024) < 1) {
    perror ("mailout: read");
    exit(1);
  }

#ifdef DEBUG
  printf ("%s", buf);
#endif

  return 0;

}

