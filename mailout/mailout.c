/* mailout.c 01/Nov/2000 (c) Jeremy C. Reed */

#include "includes.h"
#include "config.h"
#include "mailout.h"

main(argc, argv)
  int argc;
  char *argv[];
{
  int sock;
  char *mailserver;
  char *hname;
  char *temp_hostname;
  char *temp_from;
  char hostname[MAXHOSTNAMELEN + 1];

  if (argc == 1) {
    fprintf (stderr,
      "mailout: neither action flags nor mail addresses given.\n");
    exit(1);
  }

  if (parse_arguments(argc, argv)) {
    fprintf (stderr, "mailout: incomplete or unknown argument(s).\n");
    exit(1);
  }

  if (do_queue) exit(run_queue());

  if (to == NULL && !header_recipients) {
    fprintf (stderr, "mailout: recipient names must be specified.\n");
    exit(1);
  }

#ifdef DEBUG
  printf ("mailing to: %s\n", to);
#endif

  if (read_and_save_message()) {
    exit(1);
  }

/*  if (temp_hostname = strchr(to, '@')) {
    temp_hostname++;
  }
*/

  if (from == NULL) {
    from = getname();
  }

  memset(hostname, '\0', MAXHOSTNAMELEN);

  if (strchr(from, '@') == 0) {

    temp_from = from;

    /* later if hostname doesn't work use a default */
/*      if (gethostname(hostname, sizeof(hostname)) < 1) perror("gethostname"); */
      (void)gethostname(hostname, sizeof(hostname));

    from = malloc(strlen(temp_from) + strlen(hostname) + 3);
/* be sure check all malloc's */
    if (from) {
      if (sprintf(from, "%s@%s", temp_from, hostname) == 0) perror ("sprintf");
    }
    else {
      perror("malloc");
    }
  }

  if (!to) {
    /* should never get here -- this is checked earlier */
    fprintf (stderr,
      "mailout: no recipient addresses found.\n");
    exit(1);
  }

  if (strchr(to, '@') == 0) {
    temp_from = to;

    /* later if hostname doesn't work use a default */
/*      if (gethostname(hostname, sizeof(hostname)) < 1) perror("gethostname");
*/                  
      if (!hostname) (void)gethostname(hostname, sizeof(hostname));
  
    to = malloc(strlen(temp_from) + strlen(hostname) + 3); 
/* be sure check all malloc's */
    if (to) {     
      if (sprintf(to, "%s@%s", temp_from, hostname) == 0) perror ("sprintf");
    } 
    else {
      perror("malloc");
    }               
  } 

fprintf(stderr, "here 2\n");

  hname = (char *)malloc(255);
  sprintf(hname, "iwbc.net");

  mailserver = getmailserver(hname);

  free (hname);

  sock = makeconnection(mailserver);

  free(mailserver); /* don't free until after logged and used */

  if (start_smtp(sock)) exit(1); 

  if (send_message(sock)) exit(1);

  if (end_smtp(sock)) { 
    /* I assume the message was already mailed successfully, so do nothing. */ 
  }

  close(sock);

  return 0;
}

int parse_arguments(argc, argv)
  int argc;
  char *argv[];
{
  int in_options = 1;

  from = NULL;
  to = NULL;
  header_recipients = 0;
  do_queue = 0;

  while (*++argv) {
    if (in_options) {
      if (!strcmp(*argv, "-f")) {
        if (*++argv) {
          from = (char *) malloc(strlen(*argv));
          strncpy(from, *argv, strlen(*argv));
#ifdef DEBUG
          printf ("from: %s\n", from);
#endif
        }
        else return(1);
        continue;
      }
      if (!strcmp(*argv, "-q")) {
        do_queue = 1;
        continue;
      }
      if (!strcmp(*argv, "-t")) {
        header_recipients = 1;
        continue;
      }
      if (!strcmp(*argv, "--")) {
        in_options = 0;
        continue;
      }
    }

    in_options = 0;

    /* need to allow multiple recipients */
    to = (char *) malloc(strlen(*argv));
    strncpy(to, *argv, strlen(*argv));

  }

  return(0);

}

char * getmailserver (hname)
  char *hname;
{
  char *ret = NULL;

  ret = (char *)malloc(255);
  sprintf(ret, "pilchuck.reedmedia.net");

  return ret;

}

