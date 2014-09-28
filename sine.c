#include <plotpg.h>

/*
* A smoke test function.
*/
PG_FUNCTION_INFO_V1(sine);
Datum		sine(void) {
	FILE *pf;
	char buf[500];
	char result[10000] = "";

	pf = popen("gnuplot -e 'set terminal dumb; plot sin(x)'", "r");
	
	if(!pf){
		strcpy(result, "error");
	}
	
	while (fgets(buf, 500, pf) != NULL) {
		strcat(result, buf);
	}
		
	if (pclose(pf) != 0) {
		strcpy(result, "error failed to close");
	}
	
	PG_RETURN_TEXT_P(cstring_to_text(result));
}
