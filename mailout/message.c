/* message.c 04/Nov/2000 (c) Jeremy C. Reed */

/* copy standard input to a queue file */

#include <sys/types.h>
#include <sys/param.h>
/* #include <sys/syslimits.h> */
#include <fcntl.h>
#include <time.h>
#include <err.h>

#include "includes.h"
#include "config.h"
#include "mailout.h"

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
  char header_buf[80]; /* I assume no Header: definition name is long */
  int header, has_date, has_message_id;

  /* need to get a unique identifier for filename and for message */

  make_queue_filenames();

  /* need to make sure that a Message-Id header exists */

  /* need to make sure queue file doesn't already exist */

  queue_message_fd =
    open(queue_message_filename, O_WRONLY | O_TRUNC | O_CREAT | O_EXCL | O_EXLOCK, 0660);

  if (queue_message_fd == -1) {
    warn("problem with creating new queue message file %s",
         queue_message_filename);
    return (1);
  }

  queue_data_fd =
    open(queue_data_filename, O_WRONLY | O_TRUNC | O_CREAT | O_EXCL | O_EXLOCK, 0660);

  if (queue_data_fd == -1) {
    warn("problem with creating new queue data file %s", queue_data_filename);
    return (1);
  }

  return_value = 0;
  header = 1; /* assume message starts with header */
  has_date = 0;
  has_message_id = 0;
  char_count = 0;
  memset(header_buf, '\0', 80);
  memset(current_address, '\0', 300);

/* read one character at a time */
/* keep appending character to a string (up to 80 characters)
   if this string doesn't have a Header: then its not a header
*/

  while (return_value == 0 && (rcount = read(STDIN_FILENO, buf, 1)) > 0) {

    if (header) {
if ('\n' == buf[0]) printf("has newline\n");
      if ('\n' != buf[0] && char_count < 79) {
        header_buf[char_count] = buf[0];
        char_count++;
      }
      else {
/* check for Header: pattern */

#ifdef DEBUG
        fprintf(stderr, "%d-%s-\n", char_count, header_buf);
#endif

/* headers can be extended on a second line if start with a white space */
        if (header_buf[0] == '\t' || header_buf[0] == ' ' ||
            memchr(header_buf, ':', 80)) {
          if (strncmp(header_buf, "Date: ", 6) == 0) has_date = 1;
          else if (strncmp(header_buf, "Message-ID: ", 12) == 0)
            has_message_id = 1;
          else if (header_recipients) {
            tmp_pos = 0;
            if (strncmp(header_buf, "To:", 3) == 0) {
              in_to_cc_bcc = 1;
              tmp_pos = 3;
            }
            else if (strncmp(header_buf, "Cc:", 3) == 0) {
              in_to_cc_bcc = 1;
              tmp_pos = 3;
            }
            else if (strncmp(header_buf, "Bcc:", 4) == 0) {
              in_to_cc_bcc = 1;
              tmp_pos = 4;
            }
            else if (in_to_cc_bcc && (header_buf[0] == '\t' ||
                                  header_buf[0] == ' ')) in_to_cc_bcc = 1;
            else in_to_cc_bcc = 0;

            if (in_to_cc_bcc) {
             
              while (header_buf[tmp_pos] == ' ') tmp_pos++; 

              for ( ; tmp_pos < 79 && header_buf[tmp_pos] ; tmp_pos++) {
/*                if (header_buf[tmp_pos] == '<') {
                  continue;
                }
*/
                if (header_buf[tmp_pos] == '\0' ||
/*                    header_buf[tmp_pos] == '\n' || */
/*                    header_buf[tmp_pos] == ' ' || */
/*                    header_buf[tmp_pos] == '>' || */
                    header_buf[tmp_pos] == ',') {
                  if (in_address) {
#ifdef DEBUG
                    fprintf(stderr, "1recipient: %s\n", current_address);
#endif
                    wcount = write (queue_data_fd,
                           (const void *)current_address,
                           strlen(current_address)); 
                    if (wcount == -1 || wcount != strlen(current_address)) {
                      warn("problem with writing to queue data file %s",
                      queue_data_filename);
                      return_value = 1;
                      break;    
                    }
                    write (queue_data_fd, "\n", 1);
                    memset(current_address, '\0', 300);
                    current_address_len = 0; 
                    in_address = 0;
                  }
                  continue;
                }
                if (current_address_len > 300) {
                  fprintf(stderr, "mailout: recipient address to long.\n");
                  return_value = 1;
                  break;
                }
                current_address[current_address_len] = header_buf[tmp_pos];
                in_address = 1;
                current_address_len++; 
              }

            }
            else in_to_cc_bcc = 0; 
          }
        char_count = 0;
        memset(header_buf, '\0', 80);

        }
        else {
          header = 0;
          in_to_cc_bcc = 0;
        }

        if (in_to_cc_bcc && in_address) {
/* output recipient just in case */
#ifdef DEBUG
         fprintf(stderr, "2recipient: %s\n", current_address);
#endif
         wcount = write (queue_data_fd,
                         (const void *)current_address,
                         strlen(current_address)); 
         if (wcount == -1 || wcount != strlen(current_address)) {
           warn("problem with writing to queue data file %s",
             queue_data_filename);
           return_value = 1;
           break;    
         } 
         write (queue_data_fd, "\n", 1);
         memset(current_address, '\0', 300);
         current_address_len = 0; 
         in_address = 0;
        }
      }  

    } 

    wcount = write(queue_message_fd, buf, rcount);
    if (wcount == -1 || wcount != rcount) {
      warn("problem with writing to queue message file %s",
           queue_message_filename);
      return_value = 1;
      break;
    }

  }

  if (rcount < 0) {
    warn("problem reading from standard input");
    return_value = 1;
  }

  if (close(queue_message_fd)) {
    return_value = 1;
    warn("problem with closing queue message file %s", queue_message_filename);
    return (1);
  }

  if (!has_date) {
    write (queue_data_fd, "Date: need to do\n", 17); 
/*    timer = time(NULL);
    tblock = localtime(&timer);
    printf("%s", asctime(tblock));
*/
  }
  if (!has_message_id) {
    write (queue_data_fd, "Message-ID: <", 13); 
    write (queue_data_fd, unique_id, strlen(unique_id));
    write (queue_data_fd, ">\n", 2);
  }

  if (close(queue_data_fd)) {
    return_value = 1;
    warn("problem with closing queue data file %s", queue_data_filename);
    return (1);
  }

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
          "/home/reed/src/mailout/queue/%s.m", unique_id);

  sprintf(queue_data_filename,
          "/home/reed/src/mailout/queue/%s.d", unique_id);

}
