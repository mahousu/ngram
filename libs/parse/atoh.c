/* atoh - hex number conversion. Not really needed, but trivial to do.
 */

#include <ctype.h>
#include "parse.h"

int
atoh(char *string)
{
	int answer=0;

	if (!string)
		return 0;
	while (isspace(*string))
		++string;
	while (isxdigit(*string)) {
		answer <<= 4;
		if (isdigit(*string)) {
			answer += *string - '0';
		} else if (isupper(*string)) {
			answer += *string  + 10 - 'A';
		} else if (islower(*string)) {
			answer += *string  + 10 - 'a';
		}
		++string;
	}
	return answer;
}

short
atohs(char *string)
{
	return (short) atoh(string);
}
