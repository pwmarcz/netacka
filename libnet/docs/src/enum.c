/*
 * Automated chapter numbering for Texinfo files.
 *
 * Restrictions:
 *
 *   1. Only supports @chapter and @section directives.
 *   2. The chapters and sections MUST be in their correct
 *      order, and all the sections MUST be between their
 *      parent chapter and the next chapter.
 *   3. Conditional (@ifset, @ifclear) and file inclusion
 *      (@include) directives not supported.  If you must
 *      have file inclusion, feed the program with expanded
 *      file (use -E switch to Makeinfo).
 *
 * Author: Eli Zaretskii <eliz@is.elta.co.il>
 *
 * Version: 1.1
 *
 * Last updated: 22 June, 1996
 *
 * ----------------------------------------------------------
 *
 * You can do whatever you like with this program, except:
 * (1) preventing other people (including the author) do
 * whatever they like, and (2) removing the author and
 * version info above.
 *
 * ----------------------------------------------------------
 *
 */

/*
 * Modified by George Foot, April 1998
 *
 * Changes to restrictions:
 *
 * 1. @subsection is now supported
 * 2. subsections must be in the correct order and must come 
 *    between thier parent chapter and the next chapter (i.e.
 *    order things as if it were a book).
 * 3. No changes.
 *
 * Change list:
 *   - subsection support added
 *   - miscellaneous typos corrected
 *
 */


#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef  __DJGPP__

/* Make so our start-up code is minimal: disable filename
   globbing, and don't load environment file.  */
#include <crt0.h>

char ** __crt0_glob_function(char *arg) { return (char **)0; }
void   __crt0_load_environment_file(char *app_name) {}

#else

/* Some Unix boxes don't have function prototypes on the header files.
   -Wall will complain about this, so here are the prototypes:  */

void perror (const char *);
int  fgetc  (FILE *);
int  fputs  (const char *, FILE *);
int  fprintf(FILE *, const char *, ...);
int  fclose (FILE *);

#endif  /* __DJGPP__ */

static char chapter[] = "@chapter";
static char section[] = "@section";
static char subsec[] = "@subsection";
static char subsubsec[] = "@subsubsection";

int
main(int argc, char *argv[])
{
  if (argc == 3)
	{
	  size_t maxline = 100;     /* not necessarily a limit, see below */
	  char *linebuf = (char *)malloc(maxline);
	  int chap_no  = 0, sec_no = 0, subsec_no = 0, subsubsec_no = 0;
	  FILE *fp_in  = fopen(argv[1], "r");
	  FILE *fp_out = fopen(argv[2], "w");

	  if (linebuf == (char *)0)
		{
		  errno = ENOMEM;
		  perror("line storage allocation");
		  return 2;
		}
	  if (fp_in == (FILE *)0)
		{
		  perror(argv[1]);
		  return 2;
		}
	  if (fp_out == (FILE *)0)
		{
		  perror(argv[2]);
		  return 2;
		}

	  while (fgets(linebuf, maxline, fp_in))
		{
		  size_t linelen = strlen(linebuf);

		  /* If this line is longer than linebuf[], enlarge the
			 buffer and read until end of this line.  */
		  while (linebuf[linelen - 1] != '\n')
			{
			  maxline *= 2;
			  linebuf = (char *)realloc(linebuf, maxline);
			  if (linebuf == (char *)0)
				{
				  errno = ENOMEM;
				  perror("line storage re-allocation");
				  return 2;
				}

			  while (linelen < maxline && linebuf[linelen - 1] != '\n')
				{
				  linebuf[linelen++] = fgetc(fp_in);

				  if (feof(fp_in))
					linebuf[linelen-1] = '\n';
				}

			  linebuf[linelen] = '\0';
			}

		  if (memcmp(linebuf, chapter, sizeof(chapter) - 1) == 0)
			{
			  chap_no++;
			  sec_no = 0;
			  subsec_no = 0;
			  subsubsec_no = 0;
			  fputs(chapter, fp_out);
			  fprintf(fp_out, " %d.", chap_no);
			  fputs(linebuf + sizeof(chapter) - 1, fp_out);
			}
		  else if (memcmp(linebuf, section, sizeof(section) - 1) == 0)
			{
			  sec_no++;
			  subsec_no = 0;
			  subsubsec_no = 0;
			  fputs(section, fp_out);
			  fprintf(fp_out, " %d.%d", chap_no, sec_no);
			  fputs(linebuf + sizeof(section) - 1, fp_out);
			}
		  else if (memcmp(linebuf, subsec, sizeof(subsec) - 1) == 0)
			{
			  subsec_no++;
			  subsubsec_no = 0;
			  fputs(subsec, fp_out);
			  fprintf(fp_out, " %d.%d.%d", chap_no, sec_no, subsec_no);
			  fputs(linebuf + sizeof(subsec) - 1, fp_out);
			}
		  else if (memcmp(linebuf, subsubsec, sizeof(subsubsec) - 1) == 0)
			{
			  subsubsec_no++;
			  fputs(subsubsec, fp_out);
			  fprintf(fp_out, " %d.%d.%d.%d", chap_no, sec_no, subsec_no, subsubsec_no);
			  fputs(linebuf + sizeof(subsubsec) - 1, fp_out);
			}
		  else
			fputs(linebuf, fp_out);
		}

	  if (feof(fp_in))
		{
		  fprintf(stderr, "%s has %d chapters\n", argv[1], chap_no);
		  fclose(fp_in);
		  fclose(fp_out);
		  return 0;
		}
	  if (ferror(fp_in))
		{
		  perror(argv[1]);
		  return 2;
		}

	  fprintf(stderr, "Not EOF and not Error???\n");
	  return 2;
	}
  else
	{
	  fprintf(stderr, "Usage: %s infile outfile\n", *argv);
	  return 1;
	}
}
