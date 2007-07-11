#include <ctype.h>
#include <locale.h>
#include <string.h>
#include <unistd.h>

#define ISODIGIT(c)	((int)(c) >= '0' && (int)(c) <= '7')

// unescape - return string with escaped character sequences replaced by the actual characters
// that the escape codes refer to.
char * unescape(char *orig)
{
	char c, *cp, *result = orig;
	int i;

	for (cp = orig; (*orig = *cp); cp++, orig++) {
		if (*cp != '\\')
			continue;

		switch (*++cp) {
		case 'a':	/* alert (bell) */
			*orig = '\a';
			continue;
		case 'b':	/* backspace */
			*orig = '\b';
			continue;
		case 'e':	/* escape */
			*orig = '\e';
			continue;
		case 'f':	/* formfeed */
			*orig = '\f';
			continue;
		case 'n':	/* newline */
			*orig = '\n';
			continue;
		case 'r':	/* carriage return */
			*orig = '\r';
			continue;
		case 't':	/* horizontal tab */
			*orig = '\t';
			continue;
		case 'v':	/* vertical tab */
			*orig = '\v';
			continue;
		case '\\':	/* backslash */
			*orig = '\\';
			continue;
		case '\'':	/* single quote */
			*orig = '\'';
			continue;
		case '\"':	/* double quote */
			*orig = '"';
			continue;
		case '0':
		case '1':
		case '2':
		case '3':	/* octal */
		case '4':
		case '5':
		case '6':
		case '7':	/* number */
			for (i = 0, c = 0;
			     ISODIGIT((unsigned char)*cp) && i < 3;
			     i++, cp++) {
				c <<= 3;
				c |= (*cp - '0');
			}
			*orig = c;
			continue;
		case 'x':	/* hexidecimal number */
			cp++;	/* skip 'x' */
			for (i = 0, c = 0;
			     isxdigit((unsigned char)*cp) && i < 2;
			     i++, cp++) {
				c <<= 4;
				if (isdigit((unsigned char)*cp))
					c |= (*cp - '0');
				else
					c |= ((toupper((unsigned char)*cp) -
					    'A') + 10);
			}
			*orig = c;
			continue;
		default:
			--cp;
			break;
		}
	}

	return (result);
}

