/* mailout.c 01/Nov/2000 (c) Jeremy C. Reed */

#include "includes.h"
#include "config.h"
#include "mailout.h"

/* need to make prototypes */
/* int parse_arguments(int argc, char * argv); */

int main(argc, argv)
  int argc;
  char *argv[];
{
  int sock;
  char *mailserver;
/*  char *hname; */
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

#ifdef HOSTNAME
/* todo: check return value here */
  strncpy(myhostname, HOSTNAME, strlen(HOSTNAME));
  myhostname[strlen(HOSTNAME)] = '\0';
#else
  if (gethostname(myhostname, sizeof(myhostname)) < 0) {
    perror("gethostname");
    return(1);
  }
#endif

  if (!from) {
    if (NULL == (from = strdup((char *)getname()))) perror("malloc");
  }

  if (strchr(from, '@') == 0) {

    temp_from = from;

    from = malloc(strlen(temp_from) + strlen(myhostname) + 3);
/* be sure check all malloc's */
    if (from) {
      if (sprintf(from, "%s@%s", temp_from, myhostname) == 0) perror ("sprintf");
#ifdef DEBUG
    printf ("from is %s\n", from);
#endif
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
    printf ("sending to (%d) \"%s\"\n", loop, recipients[loop]);
#endif

/* this assumes that the mail server can handle all the recipients */
/* todo: fix this! this is a bad assumption! */
    if (rcpt_to(sock, recipients[loop])) {
/* some problem? should I break? Should I continue? */      
/* todo: if a problem, then note in queue data file */
/* rcpt_to() should return values depending on what it did
   for example, it shouldn't try to send message if email was rejected
   (then it should keep a counter).
   Or, it should send DATA if it has maximum recipients.
 Or should this just do one RCPT TO per message and one DATA per recipient?
*/
    }

    free(recipients[loop]);
  }

  if (send_message(sock)) exit(1);

  if (end_smtp(sock)) { 
    /* I assume the message was already mailed successfully, so do nothing. */ 
  }

  close(sock);

  /* TODO: if there is a problem log appropriately */

  /* todo: should check if all recipients were successful before: */
  /* remove queue files */
  if (unlink(queue_data_filename)) {
    /* if it doesn't remove, then what should it report? */
  }
  if (unlink(queue_message_filename)) {
    /* if it doesn't remove, then what should it report? */
  }

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
#ifdef DEBUG
    printf ("ARG / in_options: %s / %d\n", *argv, in_options);
#endif
    if (in_options) {
      if (*argv[0] == '-') { 
        if (!strcmp(*argv, "-f")) {
          if (*++argv) {
            from = (char *) malloc(strlen(*argv));
            strncpy(from, *argv, strlen(*argv));
            from[strlen(*argv)] = '\0';
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
        if ((!strcmp(*argv, "-i")) || (!strcmp(*argv, "-oi"))) {
          /* need to do? or does it matter */
          /* -i     Ignore  dots alone on lines */
          /* a dot on a line by itself should not
             terminate an incoming, non-SMTP message. */
          continue;
        }
        if (!strcmp(*argv, "--")) {
          in_options = 0;
          continue;
        }
        /* invalid option? */ 
	/* instead of return(1) just ignore */
	continue;
      } 
    }

    if (header_recipients) break; /* don't use command-line recipients */

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

/* todo: should check??
  if (strlen(MAILHUB) > MAXHOSTNAMELEN) {
    printf (stderr,
      "mailout: MAILHUB too long.\n");
    exit(1);
  }
*/
  if ((ret = malloc(MAXHOSTNAMELEN)) == NULL) {
    perror("getmailserver");
    exit(1);
  }

/* todo: need to check sizes first */
  strncpy(ret, MAILHUB, strlen(MAILHUB));
  ret[strlen(MAILHUB)] = '\0';

  return ret;

}


char * getname()
{
  struct passwd *pw;

/* todo: should use getenv() for MAILUSER, USER, LOGNAME or then ... */

  if ((pw = getpwuid(getuid())) != NULL)
    return pw->pw_name;
  else return ((char *) 0);

}

