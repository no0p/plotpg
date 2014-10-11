#include <plotpg.h>

/*
 * The gnuplot() function provides a direct way to generate plots with gnuplot
 *   commands.  This is primarily provided for flexibility for gnuplot power
 *   users.  
 *
 * A user could, for example, build their own data set with a COPY command, and
 *   subsequently provide a gnuplot script which leverages the data set.
 *
 * Another use case could be a user that wishes to plot analytic functions.
 *
 */

PG_FUNCTION_INFO_V1(gnuplot);
Datum gnuplot(PG_FUNCTION_ARGS) {
	FILE *f;
	
	StringInfoData gnuplot_script_filename;
	StringInfoData gnuplot_script_buf;
	StringInfoData result_set_line;
	StringInfoData output_filename;
	StringInfoData gnuplot_command;
	StringInfoData resultbuf;
	
	int i, ret;
	
	char pid_str[40];	
	char line[80];
	char *gnuplot_cmds = text_to_cstring(PG_GETARG_TEXT_PP(0));
	

	/*
	 *  Prep variables for temporary files
	 */
	sprintf(pid_str, "%ld", (long) getpid());
	
  initStringInfo(&output_filename);
	appendStringInfo(&output_filename, "/tmp/plotpg_%s.output", pid_str);

	initStringInfo(&gnuplot_script_filename);
	appendStringInfo(&gnuplot_script_filename, "/tmp/plotpg_%s.script", pid_str);
	
	initStringInfo(&gnuplot_command);
	appendStringInfo(&gnuplot_command, "gnuplot %s", gnuplot_script_filename.data);
	
	initStringInfo(&result_set_line);
	initStringInfo(&resultbuf);
	initStringInfo(&gnuplot_script_buf);

	
	/*
	 * 
	 * Write Gnuplot script 
	 *
	 *  TODO consider adding the application of GUCs to the script in a 
	 *    shared function also used by plot().
	 */
	f = fopen(gnuplot_script_filename.data, "w");
	
	if (f == NULL) {
		ereport(ERROR,
				(errcode(58030), //ioerror
				 errmsg("Unable to write temporary gnuplot script file in /tmp")));
	  PG_RETURN_TEXT_P(cstring_to_text(""));
	}
	
	if (strcmp(gnuplot_terminal, "") != 0)
		appendStringInfo(&gnuplot_script_buf, "set terminal %s;", gnuplot_terminal);
	
	appendStringInfo(&gnuplot_script_buf, "set output '%s';", output_filename.data);	
	appendStringInfoString(&gnuplot_script_buf, gnuplot_cmds);	
	
	fprintf(f, "%s", gnuplot_script_buf.data);
	fclose(f);
	
	/* Execute Gnuplot script */
	ret = system(gnuplot_command.data);
	elog(LOG, "%d", ret);
		
	/* Read and return output */
	f = fopen(output_filename.data, "rb");
	i = 0;
	while(fgets(line, 80, f) != NULL) {
  	if ((strcmp(gnuplot_terminal, "dumb") != 0) || i > 0) {
			appendStringInfoString(&resultbuf, line);
		}
		i++;
	}
	fclose(f);
	
	/* Cleanup */
	if (plotpg_persist < 1) {
		remove(gnuplot_script_filename.data);
		remove(output_filename.data);
	}
	
	PG_RETURN_TEXT_P(cstring_to_text(resultbuf.data));
}
