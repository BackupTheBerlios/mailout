/* recipient.c 01/Nov/2000 (c) Jeremy C. Reed */

#include "includes.h"
#include "config.h"
#include "mailout.h"

int add_recipient (address)
  char * address;
{
  char * tmp_address;
/*  int wcount; */

#ifdef DEBUG
  fprintf(stderr, "recipient: \"%s\"\n", address);
#endif

  if (NULL == (recipients =
    realloc(recipients, (1 + recipient_count) * sizeof(char *)))) {
    warn("malloc");
    return(1);
  }

  /* chop off whitespace from beginning of address -- does it really matter? */
  /* should I check if I am killing spaces forever? */
  while (address[0] == ' ' || address[0] == '\t') address++;

  /* chop off linefeed from end of address */
  if ((address[strlen(address)-1] == '\n')) address[strlen(address)-1] = '\0';

  if ((tmp_address = strchr(address, '<'))) {
    address = tmp_address;
    if (strlen(address) > 1) address++;
    if (address[strlen(address)-1] == '>') address[strlen(address)-1] = '\0';
  }

  if (NULL == (recipients[recipient_count] =
    malloc(strlen(address)+1))) {
    warn("malloc");
    return(1);
  }

/* todo: check return value or (void) */
  strncpy(recipients[recipient_count], address, strlen(address));
  recipients[recipient_count][strlen(address)] = '\0';

#ifdef DEBUG
  fprintf(stderr, "recipient added (%d): \"%s\" (length: %d)\n",
          recipient_count,
          recipients[recipient_count], strlen(recipients[recipient_count]));
#endif

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

/*  memset(address, '\0', 300); */

  return(0);
}

