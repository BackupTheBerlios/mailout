/* mailout.h 04/Nov/2000 (c) Jeremy C. Reed */


extern char * to;
extern char * from;

char * to;
char * from;

char unique_id[MAXPATHLEN + 1];
char queue_file[MAXPATHLEN + 1];

char * getmailserver (char *hname);
void make_queue_filename ();
