/* smpt.c 03/Nov/2000 (c) Jeremy C. Reed */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/param.h>
#include <fcntl.h>

#include "mailout.h"
#include "includes.h"
#include "config.h"

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

int start_smtp (s)
  int s;
{
  char tmpbuf[1024];
  int loop;

  memset(tmpbuf, '\0', 1024);

  /* mail server should send "220" banner message */
  if (read(s, tmpbuf, 1024) < 1) {
    perror ("mailout: read");
    return(1);
  }

#ifdef DEBUG
  printf ("%s", tmpbuf);
#endif

  if (strncmp("220 ", tmpbuf, 4)) {
    perror ("mailout: not smtp");
    return(1);
  }

  write (s, "HELO test\r\n", 11);

  memset(tmpbuf, '\0', 1024);

/* should reply with 250 */
  if (read(s, tmpbuf, 1024) < 1) {
    perror ("mailout: read");
    return(1);
  }

#ifdef DEBUG
  printf ("%s", tmpbuf);
#endif

  if (strncmp("250 ", tmpbuf, 4)) {
    perror ("mailout: remote smtp didn't like HELO");
    return(1);
  }

  return (0);
}


int send_message (s)
  int s;
{
  int fd, rcount, wcount, return_value;
  char buf[MAXBSIZE];
  char tmpbuf[1024];
  
  memset(tmpbuf, '\0', 1024);
  
  write (s, "MAIL FROM: ", 11);
  write (s, from, sizeof(from));
  write (s, "\n", 1);
    
/* should reply with 250 */
  if (read(s, tmpbuf, 1024) < 1) {
    perror ("mailout: read");
    return(1);
  } 

#ifdef DEBUG
  printf ("%s", tmpbuf);
#endif

  if (strncmp("250 ", tmpbuf, 4)) {
    perror ("mailout: remote smtp didn't like MAIL FROM");
    return(1);  
  }

  memset(tmpbuf, '\0', 1024);
  
  write (s, "RCPT TO: ", 8);
  write (s, to, sizeof(to));
  write (s, "\n", 1);
    
/* should reply with 250 */
  if (read(s, tmpbuf, 1024) < 1) {
    perror ("mailout: read");
    return(1);
  } 

#ifdef DEBUG
  printf ("%s", tmpbuf);
#endif

  if (strncmp("250 ", tmpbuf, 4)) {
    perror ("mailout: remote smtp didn't like RCPT TO");
    return(1);  
  }

  memset(tmpbuf, '\0', 1024);
  
  write (s, "DATA\n", 5);
    
/* should reply with 354 */
  if (read(s, tmpbuf, 1024) < 1) {
    perror ("mailout: read");
    return(1);
  } 

#ifdef DEBUG
  printf ("%s", tmpbuf);
#endif

  if (strncmp("354 ", tmpbuf, 4)) {
    perror ("mailout: remote smtp didn't like DATA command");
    return(1);  
  }

  fd = open(queue_file, O_RDONLY, 0);
  if (fd < 0) {
    warn("problem opening queue file %s", queue_file);
    return(1);
  }

  return_value = 0;

  while ((rcount = read(fd, buf, MAXBSIZE)) > 0) {
#ifdef DEBUG
fprintf(stderr, "line length: %d\n", rcount);
#endif
    if (memchr(buf, '.', 1)) {
      write (s, ".", 1); /* should test success */
#ifdef DEBUG
      fprintf(stderr, "found dot in first character\n");
#endif
    }
    wcount = write(s, buf, rcount);
    if (wcount == -1 || wcount != rcount) {
#ifdef DEBUG
      warn("problem with mailing message");
#endif
      return_value = 1;
      break;
    }
  }

  if (rcount < 0) {
    warn("problem reading from queue file", queue_file);
    return_value = 1;
  }

  if (write (s, "\r\n.\r\n", 5) != 5) {
#ifdef DEBUG
    warn("problem with sending . terminator");
#endif
    return_value = 1;
  }

  if (close(fd)) {
    return_value = 1;
    warn("problem with closing queue file %s", queue_file);
    return (1);
  }

  memset(tmpbuf, '\0', 1024);

/* should reply with 250 */
  if (read(s, tmpbuf, 1024) < 1) {
    perror ("mailout: read");
    return_value = 1;
  }
  
#ifdef DEBUG
  printf ("%s", tmpbuf);
#endif
  
  if (strncmp("250 ", tmpbuf, 4)) {
    perror ("mailout: remote smtp didn't like HELO");
    return(1);       
  } 

  return (return_value);
}

int end_smtp (s)
  int s;
{
  char buf[1024];
  int loop;

  memset(buf, '\0', 1024);

  write (s, "QUIT\r\n", 6);

/* should reply with 221 */
  if (read(s, buf, 1024) < 1) {
    perror ("mailout: read");
    return(1);
  }

#ifdef DEBUG
  printf ("%s", buf);
#endif

  if (strncmp("221 ", buf, 4)) {
    perror ("mailout: remote smtp didn't like QUIT");
    return(1);
  }

  return 0;

}

