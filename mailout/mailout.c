/* mailout.c 01/Nov/2000 (c) Jeremy C. Reed */

#include "includes.h"
#include "config.h"
#include "mailout.h"
#include "getname.c"

main(argc, argv)
  int argc;
  char *argv[];
{
  int sock;
  char *mailserver;
  char *hname;
  char *temp_hostname;
  char *temp_from;
  char *hostname;

  if (argc == 1) {
    fprintf (stderr, "mailout: neither action flags nor mail addresses given.\n");
    exit(1);
  }

  if (parse_arguments(argc, argv)) {
    fprintf (stderr, "mailout: incomplete or unknown argument(s).\n");
    exit(1);
  }

  if (to == NULL) {
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
    temp_from = getname();

printf ("1.\n");
    if (hostname = malloc(MAXHOSTNAMELEN)) {
printf ("2.\n");
    /* later if hostname doesn't work use a default */
      if (gethostname(hostname, sizeof(hostname)) < 1) perror("gethostname");
    }
    else perror("malloc");
printf ("3. %s\n", hostname);

    from = malloc(strlen(temp_from) + strlen(hostname) + 3);
printf ("4.\n");
/* be sure check all malloc's */
    if (from) {
      from = sprintf("%s@%s", temp_from, hostname);
printf ("5.\n");
    }
    else {
printf ("6.\n");
      perror("malloc");
    }
  }

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
  from = NULL;

  while (*++argv) {
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

