
#include <unistd.h>
#include <dirent.h>


DIR* dirp; 


          len = strlen(name);
          dirp = opendir(".");
          if (dirp == NULL) {
              return NOT_FOUND;
          }
          while ((dp = readdir(dirp)) != NULL) {
              if (dp->d_namlen == len && !strcmp(dp->d_name, name)) {
               closedir(dirp);
               return FOUND;
              }
          }
          closedir(dirp);
          return NOT_FOUND;

