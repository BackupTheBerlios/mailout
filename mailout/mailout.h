/* mailout.h 04/Nov/2000 (c) Jeremy C. Reed */

#define VERSION "0.14"

#ifndef MAXBSIZE
# define MAXBSIZE 65536
#endif

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

int add_recipient(char * address);
int parse_arguments();
int run_queue();
int read_and_save_message();
int makeconnection(char * hname);
int start_smtp (int s);
int rcpt_to (int s, char * address);
int send_message (int s);
int end_smtp (int s);

