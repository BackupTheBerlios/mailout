/* message.c 04/Nov/2000 (c) Jeremy C. Reed */

/* copy standard input to a queue file */

#include <sys/types.h>
#include <sys/param.h>
/* #include <sys/syslimits.h> */
#include <fcntl.h>
#include <time.h>
#include <err.h>
#include <sys/file.h>

#include "includes.h"
#include "config.h"
#include "mailout.h"

#define MAX_LINE_SIZE 1000

int read_and_save_message()
{
  int queue_message_fd, queue_data_fd, rcount, wcount;
  int return_value, char_count;
  int in_to_cc_bcc = 0;
  int tmp_pos;
  int in_address = 0;
  char current_address[300];
  int current_address_len = 0;
  static char buf[MAXBSIZE];
  static char last_buf[MAXBSIZE];
  char line_buf[MAX_LINE_SIZE];
  int header_found, header;
  time_t timer;

  /* need to get a unique identifier for filename and for message */

  make_queue_filenames();

  /* need to make sure that a Message-Id header exists */

  /* need to make sure queue file doesn't already exist */

  queue_message_fd =
    open(queue_message_filename, O_WRONLY | O_TRUNC | O_CREAT | O_EXCL, 0660);
  /* O_EXLOCK is a BSDism, so use flock */
  (void) flock(queue_data_fd, LOCK_EX); /* no check */

  if (queue_message_fd == -1) {
    warn("problem with creating new queue message file %s",
         queue_message_filename);
    return (1);
  }

  queue_data_fd =
    open(queue_data_filename, O_WRONLY | O_TRUNC | O_CREAT | O_EXCL, 0660);
  /* O_EXLOCK is a BSDism, so use flock */
  (void) flock(queue_data_fd, LOCK_EX); /* no check */

  if (queue_data_fd == -1) {
    warn("problem with creating new queue data file %s", queue_data_filename);
    return (1);
  }

  return_value = 0;
  header_found = 0; /* if no header then body should start with blank line */
  header = 1; /* assume message starts with header */
  has_date = 0;
  char_count = 0;
  memset(line_buf, '\0', MAX_LINE_SIZE);
  memset(current_address, '\0', 300);


/* read one character at a time */
/* keep appending character to a string (up to MAX_LINE_SIZE characters)
   if this string doesn't have a Header: then its not a header
*/

  do {
    rcount = read(STDIN_FILENO, buf, 1);
    if (rcount < 0) break;

    if (rcount != 0) { /* reached EOF */
      last_buf[0] = buf[0];
      line_buf[char_count] = buf[0];
      char_count++;
    }

    if (buf[0] == '\n' || char_count >= MAX_LINE_SIZE || rcount == 0) {

/* check for Header: pattern */
/* headers can be extended on a second line if start with a white space */
      if (header) {
        if (line_buf[0] == '\t' || line_buf[0] == ' ' ||
            memchr(line_buf, ':', MAX_LINE_SIZE)) {

          header_found = 1;

          if (strncasecmp(line_buf, "date: ", 6) == 0) has_date = 1;
          else if (strncasecmp(line_buf, "message-id: ", 12) == 0)
            has_message_id = 1;
          else if (header_recipients) {
            tmp_pos = 0;
            if (strncasecmp(line_buf, "to:", 3) == 0) {
              in_to_cc_bcc = 1;
              tmp_pos = 3;
            }
            else if (strncasecmp(line_buf, "cc:", 3) == 0) {
              in_to_cc_bcc = 1;
              tmp_pos = 3;
            }
            else if (strncasecmp(line_buf, "bcc:", 4) == 0) {
              in_to_cc_bcc = 1;
              tmp_pos = 4;
            }
            else if (in_to_cc_bcc && (line_buf[0] == '\t' ||
                     line_buf[0] == ' ')) in_to_cc_bcc = 1;
            else in_to_cc_bcc = 0;

            if (in_to_cc_bcc) {
             
              while (line_buf[tmp_pos] == ' ') tmp_pos++; 

              /* skip stuff in quotes, like could have a "lastname, first" */
              /* in other words, don't let that comma mean anything! */
              if (line_buf[tmp_pos] == '"') {
                tmp_pos++;
                while (line_buf[tmp_pos] != '"' && tmp_pos < char_count)
                  tmp_pos++;
                tmp_pos++; /* get rid of last quote even if is not quote */
              }

              for ( ; tmp_pos < MAX_LINE_SIZE && line_buf[tmp_pos]; tmp_pos++) {

                if (line_buf[tmp_pos] == '\0' || line_buf[tmp_pos] == ',') {
                  if (in_address) {
                    if ((return_value = add_recipient(current_address))) break;
                    memset(current_address, '\0', 300);
                    in_address = 0;
                    current_address_len = 0; 
                  }
                  continue; /* the for loop */
                }
                if (current_address_len > 300) {
                  fprintf(stderr, "mailout: recipient address too long.\n");
                  return_value = 1;
                  break;
                }
                current_address[current_address_len] = line_buf[tmp_pos];
                in_address = 1;
                current_address_len++; 

              } /* for */

            } /* in_to_cc_bcc */

          } /* is a header recipient */
          else {
            in_to_cc_bcc = 0;
          }
       
          if (in_to_cc_bcc && in_address) {
/* output recipient just in case */
            if ((return_value = add_recipient(current_address))) break;
            in_address = 0;
            current_address_len = 0; 
            memset(current_address, '\0', 300);
          }
        } /* looks like a header */
        else {
          header = 0;
#ifdef DEBUG
fprintf(stderr, "end of possible headers \"%c\"\n", line_buf[0]);
#endif
        }
      } /* header */ 

      if (! header && ! header_found) {
        /* a header was not found, so headers will be added later,
           so this message must start with blank line to separate. */
        wcount = write(queue_message_fd, "\r\n", 2);
        if (wcount == -1 || wcount != 2) {
           warn("problem with writing CRLF to %s", queue_message_filename);
           return_value = 1;
           break;
         }
#ifdef DEBUG
fprintf(stderr, "header not found!\n");
#endif
         header_found = 1; /* because the blank line is the last header */
       }

      if (char_count > 0) {

        if (rcount != 0 && buf[0] == '\n') { /* convert newline to CRLF */
          line_buf[char_count - 1] = '\r';
          line_buf[char_count] = '\n';
          char_count++;
        }

        /* message can end with a dot on a line by itself */
        if (line_buf[0] == '.' && line_buf[1] == '\r' && line_buf[2] == '\n') {
#ifdef DEBUG
          fprintf(stderr, "dot on line by itself\n");
#endif
          if (! allow_dot) {
            /* dot means end of message -- the default */
            break;
          }
          else {
            /* this is done so when sending via SMTP DATA it doesn't end */
            /* some mail clients do this for you */
            line_buf[1] = '.'; /* escape dot so two dots in a row */
            line_buf[2] = '\r';
            line_buf[3] = '\n';
            char_count++; /* or char_count = 4 */
          }
        }

        wcount = write(queue_message_fd, line_buf, char_count);
        if (wcount == -1 || wcount != char_count) {
          warn("problem with writing to queue message file %s",
                queue_message_filename);
          return_value = 1;
          break;
        }
      }

#ifdef DEBUG
        fprintf(stderr, "%d:\"%s\"\n", char_count, line_buf);
#endif
      char_count = 0;
      memset(line_buf, '\0', MAX_LINE_SIZE);
    } /* is end-of-line or MAX_LINE_SIZE characters or EOF (so do last line) */

    /* do ... while reading standard input */
  } while (return_value == 0 && rcount > 0);

  if (rcount < 0) {
    warn("problem reading from standard input");
    return_value = 1;
  }
  else if (last_buf[0] != '\n') {  /* todo:  move this up above */
    /* make sure ends with CRLF so . (dot) will be on line by itself */
    wcount = write(queue_message_fd, "\r\n", 2);
    if (wcount == -1 || wcount != 2) {
      warn("problem with writing last CRLF to queue message file %s",
            queue_message_filename);
      return_value = 1;
    }
#ifdef DEBUG
fprintf(stderr, "just wrote CRLF and last_buf was %c\n", last_buf[0]);
#endif
  }

  if (close(queue_message_fd)) {
    return_value = 1;
    warn("problem with closing queue message file %s", queue_message_filename);
    return (1);
  }

/* create headers */

  /* add "Received:" header */
  write (queue_data_fd, "Received: (mailout)\r\n", 21);
 /* Received: from rainier (rainier.reedmedia.net) [192.168.0.2] 
        by pilchuck.reedmedia.net with esmtp (Exim 3.12 #1 (Debian))
        id 14TwcV-0003dA-00; Fri, 16 Feb 2001 17:57:07 -0800        
Received: (from reed@localhost)
        by rainier.reedmedia.net (8.11.0/8.11.0) id f1H1uwe13933;
        Fri, 16 Feb 2001 17:56:58 -0800 (PST)
*/

  if (!has_date) {
    memset(line_buf, '\0', MAX_LINE_SIZE);
    timer = time(NULL);
/* rfc timezone:
note below %z may not work with some systems
dst = timetuple[8]
offs = (time.timezone, time.timezone, time.altzone)[1 + dst]
return '%+.2d%.2d' % (offs / -3600, abs(offs / 60) % 60) 
*/
    strftime(line_buf, 79, "Date: %a, %d %b %Y %T %z\r\n", localtime(&timer)); 
    write (queue_data_fd, line_buf, strlen(line_buf)); 

#ifdef DEBUG
fprintf(stderr, "Adding Date: header %s\n", line_buf);
#endif
  }
  if (!has_message_id) {
    write (queue_data_fd, "Message-ID: <", 13); 
    write (queue_data_fd, unique_id, strlen(unique_id));
    write (queue_data_fd, ">\r\n", 3);
  }

  if (close(queue_data_fd)) {
    return_value = 1;
    warn("problem with closing queue data file %s", queue_data_filename);
    return (1);
  }

  /* need to do */
  /* clean up if queue file isn't perfect -- remove it if exists */

#ifdef DEBUG
fprintf(stderr, "got to end of read_and_save_message\n");
#endif
  return(return_value);

}

void make_queue_filenames ()
{
  pid_t unique_pid;
  time_t unique_time;

  unique_pid = getpid();

  unique_time = time(NULL);

  sprintf(unique_id, "%d-%ld", unique_pid, unique_time);

/*   I am not sure how sendmail or exim chooses filenames or
     message IDs. Here are some examples:
NAA09108
Message-Id: <200011032129.QAA13868@networksolutions.com>
13s4Fj-0006WR-00
Message-Id: <E13s4Fm-00089k-00@jcr2.iwbc.net>
OAA26027
*/

  /* need to make sure that a Message-Id header exists */

  /* need to make sure queue file doesn't already exist */

  sprintf(queue_message_filename,
          "%s/%s.m", QUEUEDIRECTORY, unique_id);

  sprintf(queue_data_filename,
          "%s/%s.d", QUEUEDIRECTORY, unique_id);

}

