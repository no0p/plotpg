#include <plotpg.h>

PG_MODULE_MAGIC;

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
	appendStringInfo(&data_filename, "/tmp/plotpg_%s.data", pid_str);

  initStringInfo(&output_filename);
	appendStringInfo(&output_filename, "/tmp/plotpg_%s.output", pid_str);

	initStringInfo(&gnuplot_script_filename);
	appendStringInfo(&gnuplot_script_filename, "/tmp/plotpg_%s.script", pid_str);
	
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






