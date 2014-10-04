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
  SPITupleTable *coltuptable;
	
	StringInfoData gnuplot_script_filename;
	StringInfoData gnuplot_script_buf;
	StringInfoData result_set_line;
	StringInfoData data_filename;
	StringInfoData output_filename;
	StringInfoData gnuplot_command;
	StringInfoData resultbuf;
	
	int ret;
	int processed;
	int i, j;

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
	
	initStringInfo(&result_set_line);
	initStringInfo(&resultbuf);
	initStringInfo(&gnuplot_script_buf);
	
	/*
	 * 
	 * Execute SQL to build data file 
	 *
	 */		
	if ((ret = SPI_connect()) < 0) {
		PG_RETURN_TEXT_P(cstring_to_text("Unable to connect to SPI"));
	}
	
	/* Retrieve the desired rows */
	ret = SPI_execute(sql_in, false, 0);
	processed = SPI_processed;
	coltuptable = SPI_tuptable;

	/* Check if everything looks ok */
	if (ret != SPI_OK_SELECT) {
		SPI_finish();
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("Unable to execute statement.")));
		PG_RETURN_TEXT_P(cstring_to_text(""));
	}
	
	if (processed <= 0) {
		SPI_finish();
		PG_RETURN_TEXT_P(cstring_to_text("No results to plot."));
	}
	
	/* Iterate over result set writing to file */
	
	f = fopen(data_filename.data, "a+");
  
  if (f == NULL) {
    fclose(f);
    SPI_finish();
		PG_RETURN_TEXT_P(cstring_to_text("Unable to open tmp file to write data."));
  }
  
	if (coltuptable != NULL) {
    for(i = 0; i < processed; i++) { // Poor man's COPY
      for(j = 1; j <= coltuptable->tupdesc->natts; j++) {
  	    
  	    // TODO Get attribute information for using, timefmt here.
  	    //   order by query index.
  	    elog(LOG, "here");
  	    //coltuptable->tupdesc->attrs[j].atttypid
  	    
  	    if (SPI_getvalue(coltuptable->vals[i], coltuptable->tupdesc, j) != NULL) {
    	    appendStringInfoString(&result_set_line, SPI_getvalue(coltuptable->vals[i], coltuptable->tupdesc, j));
    	  }
		    appendStringInfo(&result_set_line, "\t");
      }
      appendStringInfo(&result_set_line, "\n");
	    fprintf(f, "%s", result_set_line.data);  
      resetStringInfo(&result_set_line);
    }
    
  }
	
	fclose(f);
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
	
	

	// TODO interpolate data about the data set into titles, labels, etc.  Will 
	//   require change away from copy potentially.
	
	if (strcmp(gnuplot_terminal, "") != 0)
	appendStringInfo(&gnuplot_script_buf, "set terminal %s;", gnuplot_terminal);

	if (strcmp(gnuplot_title, "") != 0)
	appendStringInfo(&gnuplot_script_buf, "set title '%s';", gnuplot_title);
	
	if (strcmp(gnuplot_xlabel, "") != 0)
		appendStringInfo(&gnuplot_script_buf, "set xlabel %s;", gnuplot_xlabel);
	
	if (strcmp(gnuplot_ylabel, "") != 0)
		appendStringInfo(&gnuplot_script_buf, "set ylabel %s;", gnuplot_ylabel);
	
	if (strcmp(gnuplot_xtics, "") != 0)
		appendStringInfo(&gnuplot_script_buf, "set xtics %s;", gnuplot_xtics);
	
	if (strcmp(gnuplot_ytics, "") != 0)
		appendStringInfo(&gnuplot_script_buf, "set ytics %s;", gnuplot_ytics);
	
	if (strcmp(gnuplot_key, "") != 0)
		appendStringInfo(&gnuplot_script_buf, "set key %s;", gnuplot_key);
	
	if (strcmp(gnuplot_border, "") != 0)
		appendStringInfo(&gnuplot_script_buf, "set border %s;", gnuplot_border);
	
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
	
	if (plotpg_persist < 1) {
		remove(gnuplot_script_filename.data);
		remove(data_filename.data);
		remove(output_filename.data);
	}
	
	PG_RETURN_TEXT_P(cstring_to_text(resultbuf.data));
}






