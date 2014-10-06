#include <plotpg.h>

PG_MODULE_MAGIC;

/*
 * The plot function receives an SQL statement and generates a plot.
 *
 *  It does this roughly in a five step process.  Outline below:
 *
 *  1.  execute the sql statement
 *
 *	2.  store contents in tsv temporary file
 *
 *  3.  write a gnuplot script based on options and parameters to a temp file.
 *
 *  4.  invoke gnuplot to plot the temporary script created in step 2.
 *
 *  5.  Read the output file from step 3, and return it.
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
	

	char *field_type_name;
	char pid_str[40];	
	char line[80];
	char *sql_in = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char *gnuplot_cmds = text_to_cstring(PG_GETARG_TEXT_PP(1));
	
	int natts = 0;

	int first_y_col = 2; // default unless X is timeseries

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
	
	f = fopen(data_filename.data, "w");
  
  if (f == NULL) {
    fclose(f);
    SPI_finish();
		PG_RETURN_TEXT_P(cstring_to_text("Unable to open tmp file to write data."));
  }
  
	if (coltuptable != NULL) {

		natts = coltuptable->tupdesc->natts;

		/* 
		 *   If the first attribute is a time formatted type, set the date format
		 * for the x axis to be marked as a time data, as well as the timefmt so
		 * gnuplot can read the data and convert to internal gnuplot-psuedo-epoch-time. 
		 */
		field_type_name = SPI_gettype(coltuptable->tupdesc, 1);
		if (strcmp(field_type_name, "timestamp") == 0 || strcmp(field_type_name, "timestamptz") == 0) {
			appendStringInfoString(&gnuplot_script_buf, "set xdata time;");
			appendStringInfoString(&gnuplot_script_buf, "set timefmt '%Y-%m-%d %H:%M:%S';");
			appendStringInfoString(&gnuplot_script_buf, "set format x '%H:%M:%S';");

			first_y_col = 3; // timestamp takes up cols 1 and 2 from gnuplot perspective
		}
		
		// Set the X, Y axis and key labels ....
		for(j = 1; j <= natts; j++) {
			if (j == 1)
				appendStringInfo(&gnuplot_script_buf, "set xlabel '%s';", SPI_fname(coltuptable->tupdesc, j));
			if (j == 2)
				appendStringInfo(&gnuplot_script_buf, "set ylabel '%s';", SPI_fname(coltuptable->tupdesc, j));
			// TODO keys
		}

		/* Poor man's COPY */
    for(i = 0; i < processed; i++) { 
      for(j = 1; j <= natts; j++) {
      
  	    // append the value to the line for writing to the file.
  	    if (SPI_getvalue(coltuptable->vals[i], coltuptable->tupdesc, j) != NULL) {
    	    appendStringInfoString(&result_set_line, SPI_getvalue(coltuptable->vals[i], coltuptable->tupdesc, j));
    	  }
		    appendStringInfo(&result_set_line, "\t"); //delimiter
      }
      // write the line to the file and reset the buffer
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
	
	
	if (strcmp(gnuplot_terminal, "") != 0)
		appendStringInfo(&gnuplot_script_buf, "set terminal %s;", gnuplot_terminal);

	if (strcmp(gnuplot_size, "") != 0)
		appendStringInfo(&gnuplot_script_buf, "set size %s;", gnuplot_size);

	if (strcmp(gnuplot_title, "") != 0)
		appendStringInfo(&gnuplot_script_buf, "set title %s;", gnuplot_title);
	
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

	appendStringInfo(&gnuplot_script_buf, "plot '%s' using 1", data_filename.data);	
	for (i = 0; i < (natts - 1); i++) { //the first attribute is "using 1" above, and it's always 1.
		appendStringInfo(&gnuplot_script_buf, ":%d", first_y_col + i);	
	}
	
	
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






