/* smpt.c 03/Nov/2000 (c) Jeremy C. Reed */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#include <fcntl.h>
#include <errno.h>

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
  if (hent == NULL) { /* error */
    server.sin_addr.s_addr = inet_addr(hname);
/* todo: need to report if can't resolve hname and then die */

  } else 
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
  write (s, ">\r\n", 3);
    
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
  char datebuf[80];
  char tmpbuf[1024];
  time_t timer;
  
#ifdef DEBUG
  fprintf (stderr, "start send_message()\n");
#endif

  write (s, "DATA\r\n", 6);

#ifdef DEBUG
  fprintf (stderr, "just wrote DATA\n");
#endif
    
/* should reply with 354 */
  if (read(s, tmpbuf, 1024) < 1) {
    perror ("mailout: read");
    return(1);
  } 

#ifdef DEBUG
  fprintf (stderr, "%s", tmpbuf);
#endif

  if (strncmp("354 ", tmpbuf, 4)) {
    perror ("mailout: remote smtp didn't like DATA command");
    return(1);  
  }
#ifdef DEBUG
  else fprintf (stderr, "remote smtp is waiting for data\n");
#endif

  fd = open(queue_message_filename, O_RDONLY, 0);
  if (fd < 0) {
    warn("problem opening queue file %s", queue_message_filename);
    return(1);
  }

  return_value = 0;

 /* timedate for headers */
/*todo: this should not be done here -- because date fro from should be done
at initial queue time */
  memset(datebuf, '\0', 80);
  timer = time(NULL);        
/* rfc timezone:
note below %z may not work with some systems
dst = timetuple[8]
offs = (time.timezone, time.timezone, time.altzone)[1 + dst]
return '%+.2d%.2d' % (offs / -3600, abs(offs / 60) % 60)
*/
  strftime(datebuf, 79, "%a, %d %b %Y %T %z", localtime(&timer));

/* first send "Received: " header */

/* should this have "for" address ? */
/* todo: should this check if address and others have values first? */
  memset(tmpbuf, '\0', 1024);
  if (snprintf(tmpbuf, 1024, "Received: from %s by %s with local"
               " (mailout " VERSION ")\r\n"
               "\tid %s; %s\r\n",
               from, myhostname, unique_id,datebuf) == 0)
    perror ("snprintf"); /* should this die? */
  else {
    write (s, tmpbuf, strlen(tmpbuf));

#ifdef DEBUG
fprintf(stderr, "Just wrote Received: header\n");
#endif
  }

/* todo: this should be done somewhere else -- because this info is wrong
   if it is delivered later */

  if (!has_date) {                                           
    write (s, "Date: ", 6); 
    write (s, datebuf, strlen(datebuf));
    write (s, "\r\n", 2);

#ifdef DEBUG
fprintf(stderr, "just wrote Date: header %s\n", tmpbuf);
#endif
  }
/* send Message-Id: header if needed */
  if (!has_message_id) {
    write (s, "Message-Id: <", 13); 
    write (s, unique_id, strlen(unique_id));
#ifdef DEBUG
fprintf(stderr, "unique_id %s", unique_id);
#endif
    write (s, "@", 1);
#ifdef DEBUG
fprintf(stderr, " @myhostname %s\n", myhostname);
#endif
    write (s, myhostname, strlen(myhostname));
    write (s, ">\r\n", 3);
#ifdef DEBUG
fprintf(stderr, "Just wrote message-Id\n");
#endif
  }
#ifdef DEBUG
  else fprintf(stderr, "Didn't create new Message-Id\n");
#endif

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
    wcount = write(s, buf, rcount); /* todo: check success here */
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

  /* message.c makes sure that queued message ends with CRLF */
  if (write (s, ".\r\n", 3) != 3) {
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
  fprintf (stderr, "%s", tmpbuf);
#endif
  
  if (strncmp("250 ", tmpbuf, 4)) {
    perror ("mailout: remote smtp didn't like actual DATA");
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
#ifdef DEBUG
    printf ("received: %s", buf);
#endif
    return(1);
  }

  return 0;

}

