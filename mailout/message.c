/* message.c 04/Nov/2000 (c) Jeremy C. Reed */

/* copy standard input to a queue file */

#include <sys/types.h>
#include <sys/param.h>
/* #include <sys/syslimits.h> */
#include <fcntl.h>

#include "includes.h"
#include "config.h"
#include "mailout.h"

int read_and_save_message()
{
  int queue_fd, rcount, wcount;
  int return_value;
  static char buf[MAXBSIZE];
  int header, has_date, has_message_id;

  /* need to get a unique identifier for filename and for message */

  make_queue_filename();

  /* need to make sure that a Message-Id header exists */

  /* need to make sure queue file doesn't already exist */

  sprintf(queue_file, message_id);

  queue_fd =
    open(queue_file, O_WRONLY | O_TRUNC | O_CREAT | O_EXCL | O_EXLOCK, 0660);

  if (queue_fd == -1) {
    warn("problem with creating new queue file %s", queue_file);
    return (1);
  }

  return_value = 0;
  header = 1; /* assume message starts with header */
  has_date = 0;
  has_message_id = 0;

  /* save it as is -- handle headers later */
  while ((rcount = read(STDIN_FILENO, buf, MAXBSIZE)) > 0) {

    if (header) {
      if (memchr(buf, ':', rcount)) {
        if (strncmp(buf, "Date: ", 6) == 1) has_date = 1;
        else if (strncmp(buf, "Message-ID: ", 12) == 1) has_message_id = 1;
      }
      else {
        header = 0;
        if (!has_date) {
          write (queue_fd, "Date: need to do\n", 17); 
          has_date = 1;
        }
        if (!has_message_id) {
          write (queue_fd, "Message-ID: need to do\n", 23); 
          has_message_id = 1;
        }
      }
    }  
    

    wcount = write(queue_fd, buf, rcount);
    if (wcount == -1 || wcount != rcount) {
      warn("problem with writing to queue file %s", queue_file);
      return_value = 1;
      break;
    }
  }

  if (rcount < 0) {
    warn("problem reading from standard input");
    return_value = 1;
  }

  if (close(queue_fd)) {
    return_value = 1;
    warn("problem with closing queue file %s", queue_file);
    return (1);
  }

  /* clean up if queue file isn't perfect -- remove it if exists */

  return(return_value);

}

void make_queue_filename ()
{
  sprintf(message_id, "UNIQUE_ID");

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

  sprintf(queue_file, message_id);

}
