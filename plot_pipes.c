/*
* Takes an SQL query in a string format, executes the query, and passes the results to plotpg.  
* 
* Returns a plot in dumb or svg mode depending on the GUC.
*
* The first parameter is a string sql statement.  The second parameter is an optional
*   which is prepended to the gnuplot command.
*
* This function is here as an implementation example of using popen and pipes
*   rather than temporary files for plotting.  It turns out there are some limitations
*   to this method, and the potentially less performant temporary file solution works
*   as a general case solution.
*
*/

Datum plot_no_file(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(plot_no_file);
Datum plot_no_file(PG_FUNCTION_ARGS) {
	int ret;
	int processed;
	int i;
	int j;
	StringInfoData resultbuf;
	StringInfoData databuf;
	SPITupleTable *coltuptable;
	FILE *pf;
	char buf[5000]; //review \n behavior of gnuplot in svg mode to tune this.
	char *sql = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char *gnuplot_cmds = text_to_cstring(PG_GETARG_TEXT_PP(1));
	int in_count = 0;
	
	if ((ret = SPI_connect()) < 0) {
		PG_RETURN_TEXT_P(cstring_to_text("Unable to connect to SPI"));
	}
	
	/* Retrieve the desired rows */
	ret = SPI_execute(sql, true, 0);
	processed = SPI_processed;

	/* Check if everything looks ok */
	if (ret != SPI_OK_SELECT) {
		SPI_finish();
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Unable to execute select statement.")));
	}
	
	if (processed <= 0) {
		SPI_finish();
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("No results to plot.")));
	}
	
	if (processed > 2000) {
		SPI_finish();
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Unable to plot more than ~ 2000 results.")));
	}
	
	/* Initialize buffers for the plotting command and result, then build command*/
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
	
	appendStringInfoString(&databuf, "e\" | gnuplot -e \"");
	appendStringInfoString(&databuf, gnuplot_cmds); //parameter for set commands;

	if (strcmp(gnuplot_terminal, "svg") == 0) {
		appendStringInfoString(&databuf, "set terminal svg;");
	} else {
		appendStringInfoString(&databuf, "set terminal dumb;");
	}
	appendStringInfoString(&databuf, "plot '-' with lines\""); // using 0:2 For timeseries.  TODO autodetect timestamp.
	
	elog(LOG, "%s", databuf.data); //debugging - display full command in log.

	/* Execute plot command*/
	pf = popen(databuf.data, "r");
	
	if(!pf){
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Unable to open gnuplot pipe.")));
	}
	
	/* retrive GNU plot output*/
	while (fgets(buf, 500, pf) != NULL) {
		if (in_count > 0) {
			appendStringInfoString(&resultbuf, buf);
		}
		in_count++;
	}
	
	/* cleanup */
	if (pclose(pf) != 0) {
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Unable to close gnuplot pipe.")));
	}
	
	SPI_finish();
	PG_RETURN_TEXT_P(cstring_to_text(resultbuf.data));
}
