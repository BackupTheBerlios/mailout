/* smpt.c 03/Nov/2000 (c) Jeremy C. Reed */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
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
  int tmp_len;
  char * tmp_str;

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

  tmp_len = 8 + strlen(myhostname);
  if (NULL == (tmp_str = malloc(tmp_len))) {
    perror("malloc");
    return(1);
  } 

  if (0 == snprintf(tmp_str, tmp_len,
                    "HELO %s\r\n", myhostname)) {
    perror("snprintf");
    return(1);
  }

#ifdef DEBUG
  printf ("sent: HELO %s\r\n", myhostname);
#endif
  write (s, tmp_str, (tmp_len - 1)); 
  free(tmp_str);

  memset(tmpbuf, '\0', 1024);

/* should reply with 250 */
  if (read(s, tmpbuf, 1024) < 1) {
    perror ("mailout: read");
    return(1);
  }

#ifdef DEBUG
  printf ("returned: %s", tmpbuf);
#endif

  if (strncmp("250 ", tmpbuf, 4)) {
    perror ("mailout: remote smtp didn't like HELO");
    return(1);
  }

  memset(tmpbuf, '\0', 1024);

  tmp_len = 14 + strlen(from);
  if (NULL == (tmp_str = malloc(tmp_len))) {
    perror("malloc");
    return(1);
  }
   
  if (0 == snprintf(tmp_str, tmp_len,
                    "MAIL FROM: %s\r\n", from)) {
    perror("snprintf");
    return(1);
  }
  
#ifdef DEBUG
  printf("sent: %s", tmp_str);
#endif

  write (s, tmp_str, (tmp_len - 1));
  free(tmp_str);
  
/* should reply with 250 */
  if (read(s, tmpbuf, 1024) < 1) {
    perror ("mailout: read");
    return(1);
  } 

#ifdef DEBUG
  printf ("returned: %s", tmpbuf);
#endif

  if (strncmp("250 ", tmpbuf, 4)) {
    perror ("mailout: remote smtp didn't like MAIL FROM");
    return(1);  
  }
  return (0);
}

int rcpt_to (s, address)
  int s;
  char * address;
{
  char tmpbuf[1024];
  char * temp;

  if (strchr(address, '@') == 0) {
    temp = address;
    
    address = malloc(strlen(temp) + strlen(myhostname) + 3);
/* be sure check all malloc's */ 
    if (address) {
      if (sprintf(address, "%s@%s", temp, myhostname) == 0) perror ("sprintf");
    } 
    else {
      perror("malloc");
    }
  }

  if (NULL == to) to = strdup(address);

  memset(tmpbuf, '\0', 1024);
  
  write (s, "RCPT TO: <", 10);
  write (s, address, strlen(address));
  write (s, ">\n", 2);
    
/* should reply with 250 */
  if (read(s, tmpbuf, 1024) < 1) {
    perror ("mailout: read");
    return(1);
  } 

#ifdef DEBUG
  printf ("%s", tmpbuf);
#endif

/* need to check for 501 -- bad "rcpt to" */

  if (strncmp("250 ", tmpbuf, 4)) {
    perror ("mailout: remote smtp didn't like RCPT TO");
    return(1);  
  }

  memset(tmpbuf, '\0', 1024);

  return(0);

}

int send_message (s)
  int s;
{
  int fd, rcount, wcount, return_value;
  char buf[MAXBSIZE];
  char tmpbuf[1024];
  
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

  fd = open(queue_message_filename, O_RDONLY, 0);
  if (fd < 0) {
    warn("problem opening queue file %s", queue_message_filename);
    return(1);
  }

  return_value = 0;

  while ((rcount = read(fd, buf, MAXBSIZE)) > 0) {
#ifdef DEBUG
fprintf(stderr, "line length: %d\n", rcount);
#endif
/* need to fix */
/* this is way wrong, because it isn't line by line */
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
    warn("problem reading from queue message file %s", queue_message_filename);
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
    warn("problem with closing queue file %s", queue_message_filename);
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

