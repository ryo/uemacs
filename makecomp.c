#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

int get_alpha_pos (char *name)
{
  char ext, *p;

  for (p = name; !isalpha (ext = *p); p++) {
    if (!ext)
      return 0;
  }

  return p - name;
}

int main (int argc, char *argv[])
{
  if (argc != 3) {
    printf ("usage: makecomp <in-file> <out-file>\n");
    return 0;
  }

  {
    FILE *fp_in, *fp_out[26];
    int i;
    char *in_name, *out_name_base;
    char name[256];

    in_name = argv[1];
    out_name_base = argv[2];

    fp_in = fopen (in_name, "rb");
    if (fp_in == NULL) {
      printf ("file open error '%s'\n", in_name);
      return 1;
    }
    for (i = 0; i < 26; i++) {
      sprintf (name, "%s.em%c", out_name_base, (int)'a' + i);
      fp_out[i] = fopen (name, "wb");
      if (fp_out[i] == NULL) {
        printf ("file open error '%s'\n", name);
        return 1;
      }
    }

    {
      char ext, *result, line[1024];

      while (1) {
        result = fgets (line, 1024, fp_in);
        if (result == NULL) {
          if (feof (fp_in))
            break;
          perror ("fgets");
        }

        ext = tolower (line[get_alpha_pos (line)]);
        if (isalpha (ext)) {
          if (fwrite (line, strlen (line), 1, fp_out[ext - 'a']) != 1) {
            char errstring[256];

            sprintf (errstring, "fwrite '%s.em%c'", out_name_base, ext);
            perror (errstring);
          }
        } else
          printf ("incorrect string : %s", line);
      }
    }
  }

  return 0;
}
