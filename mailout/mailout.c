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
  char *temp_from;
  int loop;

  if (argc == 1) {
    fprintf (stderr,
      "mailout: neither action flags nor mail addresses given.\n");
    exit(1);
  }

  recipients = NULL;
  recipient_count = 0;

  if (parse_arguments(argc, argv)) {
    fprintf (stderr, "mailout: incomplete or unknown argument(s).\n");
    exit(1);
  }

  openlog("mailout", LOG_PID, LOG_MAIL);

  if (do_queue) exit(run_queue());

  if (*argv == NULL && !header_recipients) {
    fprintf (stderr, "mailout: recipient names must be specified.\n");
    exit(1);
  }

  if (read_and_save_message()) {
    exit(1);
  }

/* maybe first look at HOSTNAME via getenv() */

  if (gethostname(myhostname, sizeof(myhostname) - 1) < 0) {
    perror("gethostname");
    return(1);
  }
  myhostname[sizeof(myhostname) - 1] = '\0';

  if (!from) {
    if (NULL == (from = strdup((char *)getname()))) perror("malloc");
  }

  if (strchr(from, '@') == 0) {

    temp_from = from;

    from = malloc(strlen(temp_from) + strlen(myhostname) + 3);
/* be sure check all malloc's */
    if (from) {
      if (sprintf(from, "%s@%s", temp_from, myhostname) == 0) perror ("sprintf");
    }
    else {
      perror("malloc");
    }
  }

  if ((!*argv) && recipient_count == 0) {
    fprintf (stderr,
      "mailout: no recipient addresses found.\n");
    exit(1);
  }

  /* later get info for particular domain */
  mailserver = getmailserver("");

  sock = makeconnection(mailserver);

  free(mailserver); /* don't free until after logged and used */

  if (start_smtp(sock)) exit(1); 

  for (loop = 0; loop < recipient_count ; loop++) {
#ifdef DEBUG
    printf ("(%d) sending to %s\n", loop, recipients[loop]);
#endif

    if (rcpt_to(sock, recipients[loop])) {
/* some problem? should I break? Should I continue? */      
    }

    free(recipients[loop]);
  }

  if (send_message(sock)) exit(1);

  if (end_smtp(sock)) { 
    /* I assume the message was already mailed successfully, so do nothing. */ 
  }

  close(sock);

  syslog(LOG_INFO, "%s sent to %s", from, to);
  closelog();

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
      if (!strcmp(*argv, "-i")) {
        /* need to do? or does it matter */
        /* -i     Ignore  dots alone on lines */
        continue;
      }
      if (!strcmp(*argv, "--")) {
        in_options = 0;
        continue;
      }
    }
    else if (header_recipients) break; /* don't use command-line recipients */

    if (add_recipient(*argv)) return(1);

    in_options = 0;
  }

  return(0);

}

/* getmailserver will try to get MX or A records */
/* currently it just returns a mail hub */
char * getmailserver (hname)
  char *hname;
{
  char *ret;

  if ((ret = malloc(255)) == NULL) {
    perror("getmailserver");
    exit(1);
  }
  strncpy(ret, MAILHUB, strlen(MAILHUB));

  return ret;

}


char * getname()
{
  struct passwd *pw;

/* should use getenv() for MAILUSER, USER, LOGNAME or then ... */

  if ((pw = getpwuid(getuid())) != NULL)
    return pw->pw_name;
  else return ((char *) 0);

}


int add_recipient (address)
  char * address;
{
/*  int wcount; */
#ifdef DEBUG
  fprintf(stderr, "recipient: %s\n", address);
#endif

  if (NULL == (recipients =
    realloc(recipients, (2 * recipient_count) + 2))) {
    warn("malloc");
    return(1);
  }

  /* chop off whitespace from beginning of address -- does it really matter? */
  /* should I check if I am killing spaces forever? */
  while (address[0] == ' ' || address[0] == '\t') address++;

  if (NULL == (recipients[recipient_count] =
    malloc(sizeof(address)))) {
    warn("malloc");
    return(1);
  }

  strncpy(recipients[recipient_count], address,
          strlen(address));

  recipient_count++;

/*  wcount = write (queue_data_fd, (const void *)address,
                  strlen(address)); 
  if (wcount == -1 || wcount != strlen(address)) {
    warn("problem with writing to queue data file %s",
    queue_data_filename);
    return(1);
  }
  write (queue_data_fd, "\n", 1);
*/

  memset(address, '\0', 300);

  return(0);
}
