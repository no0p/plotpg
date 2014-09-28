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

/*
 * The plot function receives an SQL statement and generates a plot.
 *
 *  It does this in a four step process.
 *
 *  1.  execute the sql statement inside a copy command, storing the result
 *        in a temporary file.
 *
 *  2.  write a gnuplot script based on options and parameters to a temp file.
 *
 *  3.  invoke gnuplot to plot the temporary script created in step 2.
 *
 *  4.  Read the output file from step 3, and return it.
 *
*/
PG_FUNCTION_INFO_V1(plot);
Datum plot(PG_FUNCTION_ARGS) {
	FILE *f;
	
	StringInfoData gnuplot_script_filename;
	StringInfoData gnuplot_script_buf;
	StringInfoData data_query;
	StringInfoData data_filename;
	StringInfoData output_filename;
	StringInfoData gnuplot_command;
	StringInfoData resultbuf;
	
	int ret;
	int processed;
	int i;

	char pid_str[40];	
	char line[80];
	char *sql_in = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char *gnuplot_cmds = text_to_cstring(PG_GETARG_TEXT_PP(1));

	/*
	 *  Prep variables for temporary files
	 */
	sprintf(pid_str, "%ld", (long) getpid());
	
  initStringInfo(&data_filename);
	appendStringInfo(&data_filename, "/tmp/plotpg_data_%s.data", pid_str);

  initStringInfo(&output_filename);
	appendStringInfo(&output_filename, "/tmp/plotpg_%s.output", pid_str);

	initStringInfo(&gnuplot_script_filename);
	appendStringInfo(&gnuplot_script_filename, "/tmp/plotpg_script_%s.gp", pid_str);
	
	initStringInfo(&gnuplot_command);
	appendStringInfo(&gnuplot_command, "gnuplot %s", gnuplot_script_filename.data);
	
	initStringInfo(&resultbuf);

	/*
	 * 
	 * Execute SQL to build data file 
	 *
	 */	
	initStringInfo(&data_query);
	appendStringInfo(&data_query, "copy (%s) to '", sql_in);
	appendStringInfoString(&data_query, data_filename.data);
	appendStringInfoString(&data_query, "'");
	elog(LOG, "B");
	elog(LOG, "%s", data_query.data);
	
	if ((ret = SPI_connect()) < 0) {
		PG_RETURN_TEXT_P(cstring_to_text("Unable to connect to SPI"));
	}
	
	/* Retrieve the desired rows */
	ret = SPI_execute(data_query.data, false, 0);
	processed = SPI_processed;

	/* Check if everything looks ok */
	if (ret != SPI_OK_UTILITY) {
		SPI_finish();
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Unable to execute select statement.")));
		PG_RETURN_TEXT_P(cstring_to_text(""));
	}
	
	if (processed <= 0) {
		SPI_finish();
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("No results to plot.")));
	  PG_RETURN_TEXT_P(cstring_to_text(""));
	}
	SPI_finish();
	
	/*
	 * 
	 * Write Gnuplot script 
	 *
	 */
	f = fopen(gnuplot_script_filename.data, "w");
	
	if (f == NULL) {
		ereport(ERROR,
				(errcode(58030), //ioerror
				 errmsg("Unable to write temporary gnuplot script file in /tmp")));
	  PG_RETURN_TEXT_P(cstring_to_text(""));
	}
	
	// Prepare the script
	initStringInfo(&gnuplot_script_buf);

	// TODO Append all GUCs, function which returns big string? -> 
	appendStringInfo(&gnuplot_script_buf, "set terminal '%s';", gnuplot_terminal);
	appendStringInfo(&gnuplot_script_buf, "set title '%s';", gnuplot_title);
	appendStringInfo(&gnuplot_script_buf, "set xlabel '%s';", gnuplot_xlabel);
	appendStringInfo(&gnuplot_script_buf, "set ylabel '%s';", gnuplot_ylabel);
	appendStringInfo(&gnuplot_script_buf, "set output '%s';", output_filename.data);	
	
	appendStringInfoString(&gnuplot_script_buf, gnuplot_cmds);

	appendStringInfo(&gnuplot_script_buf, "plot '%s' using 1:2", data_filename.data);	
	
	fprintf(f, "%s", gnuplot_script_buf.data);
	fclose(f);
	
	/* Execute Gnuplot script */
	ret = system(gnuplot_command.data);
	
	/* Read and return output */
	f = fopen(output_filename.data, "rb");
	i = 0;
	while(fgets(line, 80, f) != NULL) {
  	// skip first line which has a bogus character in dumb mode
		if ((strcmp(gnuplot_terminal, "dumb") != 0) || i > 0) {
			appendStringInfoString(&resultbuf, line);
		}
		i++;
	}
	fclose(f);
	
	
	/* Cleanup */
	remove(gnuplot_script_filename.data);
	remove(data_filename.data);
	remove(output_filename.data);
	
	PG_RETURN_TEXT_P(cstring_to_text(resultbuf.data));
}


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

/* 
* Sets a bunch of GUCs.	Called at so load.
*
*	Includes gucs for a subset of gnuplot's set commands.
*
*/
void _PG_init(void) {

	/* Output mode for plots.	svg or dumb.	Review how to make the so load at server start. */
	DefineCustomStringVariable("terminal", "gnuplot terminals 'svg', 'dumb' supported",
                              NULL, &gnuplot_terminal, "dumb", PGC_USERSET, 0,
                              NULL, NULL, NULL);
                              
	DefineCustomStringVariable("title",
                              "Plot title",
                              NULL,
                              &gnuplot_title,
                              "",
                              PGC_USERSET,
                              0,
                              NULL,
                              NULL,
                              NULL);

	DefineCustomStringVariable("xlabel",
                              "label for the xaxis",
                              NULL,
                              &gnuplot_xlabel,
                              "",
                              PGC_USERSET,
                              0,
                              NULL,
                              NULL,
                              NULL);

DefineCustomStringVariable("ylabel",
                              "label for the y axis",
                              NULL,
                              &gnuplot_ylabel,
                              "",
                              PGC_USERSET,
                              0,
                              NULL,
                              NULL,
                              NULL);

}

