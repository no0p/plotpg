#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "executor/spi.h"
#include "lib/stringinfo.h"

#include <stdio.h>
#include <string.h>


PG_MODULE_MAGIC;

Datum	sine(void);
Datum plot(PG_FUNCTION_ARGS);
Datum stdinput(void);

PG_FUNCTION_INFO_V1(stdinput);
Datum		stdinput(void) {
	FILE *pf;
	char buf[500];
	char result[10000] = "";

	pf = popen("echo \"1 1\ne\" | gnuplot -e \"set terminal dumb; plot '-'\"", "r");
	
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

PG_FUNCTION_INFO_V1(plot);
Datum plot(PG_FUNCTION_ARGS) {
	int ret;
	int processed;
	int i;
	int j;
	StringInfoData resultbuf;
	StringInfoData databuf;
	SPITupleTable *coltuptable;
	FILE *pf;
	char buf[500];
	char *sql = text_to_cstring(PG_GETARG_TEXT_PP(0));
	int in_count = 0;
	
	if ((ret = SPI_connect()) < 0) {
		PG_RETURN_TEXT_P(cstring_to_text("Unable to connect to SPI"));
	}
	
	/* Retrieve the desired rows */
	ret = SPI_execute(sql, true, 0);
	processed = SPI_processed;

	/* If no qualifying tuples, fall out early */
	if (ret != SPI_OK_SELECT || processed <= 0) {
		SPI_finish();
		PG_RETURN_TEXT_P(cstring_to_text("Select statement failed or no results"));
	}
	
	coltuptable = SPI_tuptable;
	initStringInfo(&resultbuf);
	initStringInfo(&databuf);
	
	appendStringInfoString(&databuf, "echo \"");
	if (coltuptable != NULL) {
    for(i = 0; i < processed; i++) {
      for(j = 1; j <= coltuptable->tupdesc->natts; j++) {
				appendStringInfoString(&databuf, SPI_getvalue(coltuptable->vals[i], coltuptable->tupdesc, j));
				appendStringInfoString(&databuf, " ");
			}
			appendStringInfoString(&databuf, "\n");
		}
	}
	appendStringInfoString(&databuf, "e\" | gnuplot -e \"set terminal dumb; plot '-'\"");
	
	elog(LOG, "%s", databuf.data);
	pf = popen(databuf.data, "r");
	
	if(!pf){
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Unable to open gnuplot pipe.")));
	}
  
	while (fgets(buf, 500, pf) != NULL) {
	  if (in_count > 0) {
	  	appendStringInfoString(&resultbuf, buf);
	  }
	  in_count++;
  }
    
	if (pclose(pf) != 0) {
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Unable to close gnuplot pipe.")));
	}
	
	SPI_finish();
	PG_RETURN_TEXT_P(cstring_to_text(resultbuf.data));
}



