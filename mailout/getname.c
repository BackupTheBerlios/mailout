
#include <sys/types.h>
#include <pwd.h>
#include <unistd.h>

const char * getname()
{
  struct passwd *pw;

  if ((pw = getpwuid(getuid())) != NULL)
    return pw->pw_name;
  else return ((char *) 0);

}
