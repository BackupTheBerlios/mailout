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


  hname = (char *)malloc(255);
  sprintf(hname, "iwbc.net");

  mailserver = getmailserver(hname);

  free (hname);

  sock = makeconnection(mailserver);

  free(mailserver); /* don't free until after logged and used */

  communicate(sock);

  close(sock);

  return 0;
}

int parse_arguments(argc, argv)
  int argc;
  char *argv[];
{

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
  sprintf(ret, "mail.postalzone.com");

  return ret;

}

