/* mailout.h 04/Nov/2000 (c) Jeremy C. Reed */

#define VERSION "0.13"

extern char * to;
extern char * from;

char * to;
char * from;

char unique_id[MAXPATHLEN + 1];
char queue_message_filename[MAXPATHLEN + 1];
char queue_data_filename[MAXPATHLEN + 1];

char myhostname[MAXHOSTNAMELEN + 1];

int do_queue;
int header_recipients;

char **recipients;
int recipient_count;
int has_message_id;
int has_date;

char * getmailserver (char *hname);
void make_queue_filenames ();
char * getname();

int add_recipient();

