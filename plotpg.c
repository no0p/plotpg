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
	
	StringInfoData data_filename;
	StringInfoData output_filename;
	StringInfoData gnuplot_script_filename;
	
	StringInfoData gnuplot_script_buf;
	
	StringInfoData gnuplot_command;
	StringInfoData plot_statement;
	
	StringInfoData result_set_line;
	StringInfoData resultbuf;
	
	int ret;
	int processed;
	int i, j;
	
	char pid_str[40];	
	char line[80];
	char *sql_in = text_to_cstring(PG_GETARG_TEXT_PP(0));
	char *gnuplot_cmds = text_to_cstring(PG_GETARG_TEXT_PP(1));
	
	int natts = 0;
  int plot_type = GNUPLOT_UNKNOWN;

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
	initStringInfo(&plot_statement);
	
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
		PG_RETURN_TEXT_P(cstring_to_text("Unable to execute statement."));
	}
	
	if (processed <= 0) {
		SPI_finish();
		PG_RETURN_TEXT_P(cstring_to_text("No results to plot."));
	}
	
	if (coltuptable == NULL) {
		SPI_finish();
		PG_RETURN_TEXT_P(cstring_to_text("Empty query result."));
	}
	
	natts = coltuptable->tupdesc->natts;
	if (natts < 2) {
	  SPI_finish();
		PG_RETURN_TEXT_P(cstring_to_text("Not sure how to plot a result set with a single column.  See gnuplot() function for manual control of plotting facility."));
	}
	
	/* Set the chart type, a defined enumlist */
	plot_type = infer_chart_type(processed, 
															 SPI_gettype(coltuptable->tupdesc, 1), 
															 SPI_gettype(coltuptable->tupdesc, 2));
	
	
	/* Set the default X, Y axis labels */
	appendStringInfo(&gnuplot_script_buf, "set xlabel '%s';", SPI_fname(coltuptable->tupdesc, 1));
	appendStringInfo(&gnuplot_script_buf, "set ylabel '%s';", SPI_fname(coltuptable->tupdesc, 2));
	
	/* 
	 * Iterate over the fields to build the final plot statement
	 */
	appendStringInfo(&plot_statement, "plot '%s' using 1", data_filename.data);	
	for(j = 2; j <= natts; j++) {
		if (j == 2) {
			switch (plot_type) {
				case GNUPLOT_TIMESERIES :
					appendStringInfo(&plot_statement, ":%d", j + 1);
					break;
				default :
					appendStringInfo(&plot_statement, ":%d", j);
			}
		}
		
		if (j > 2) {
			if (strcmp(SPI_gettype(coltuptable->tupdesc, j), "numeric") == 0 ||
					 strcmp(SPI_gettype(coltuptable->tupdesc, j), "real") == 0 ||
					 strcmp(SPI_gettype(coltuptable->tupdesc, j), "float") == 0 ||
					 strcmp(SPI_gettype(coltuptable->tupdesc, j), "bigint") == 0 ||
					 strcmp(SPI_gettype(coltuptable->tupdesc, j), "int") == 0 ||
					 strcmp(SPI_gettype(coltuptable->tupdesc, j), "decimal") == 0) {
					 
				appendStringInfo(&plot_statement, ":%d", j);
				// update keys....
			}
		}
	}
	// Modify plot command with "with" type modifiers 
	
	
	/* No key if 1 dependent variable*/
	if (natts < 3) {
		appendStringInfoString(&gnuplot_script_buf, "set key off;");
	}
	
	/* Iterate over result set writing to file */
	f = fopen(data_filename.data, "w");
  
  if (f == NULL) {
    fclose(f);
    SPI_finish();
		PG_RETURN_TEXT_P(cstring_to_text("Unable to open tmp file to write data."));
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
	
	fclose(f);
	SPI_finish();
	
	/*
	 * 
	 * Write Gnuplot script.
	 * 
	 *   Add manually set variables from GUCs, gnuplot commands passed as an
	 * argument, and the explicit plot command.  Note, some gnuplot commands
	 * were likely already added when examining the data while writing it 
	 * to a gnuplot compatible tsv file in the execution section above.
	 *
	 */
	
	/* Add gnuplot commands based on the plot_type */
	switch (plot_type) {
		case GNUPLOT_HORIZ_BAR :
			appendStringInfoString(&gnuplot_script_buf, "");
			break;
		case GNUPLOT_TIMESERIES :
		  /* 
			 *   Timeseries: If the first attribute is a time formatted type, set the 
			 * date format for the x axis to be marked as a time data, as well as 
			 * the timefmt so gnuplot can read the data and convert to internal 
			 * gnuplot-psuedo-epoch-time. 
			 */
			appendStringInfoString(&gnuplot_script_buf, "set xdata time;");
			appendStringInfoString(&gnuplot_script_buf, "set timefmt '%Y-%m-%d %H:%M:%S';");
			appendStringInfoString(&gnuplot_script_buf, "set format x '%H:%M:%S';");
			appendStringInfoString(&gnuplot_script_buf, "set xtics rotate by -45 offset -1;");
			break;
		case GNUPLOT_HISTOGRAM :
			appendStringInfoString(&gnuplot_script_buf, "");
			break;
		case GNUPLOT_SCATTER :
			appendStringInfoString(&gnuplot_script_buf, "");
			break;
		default :
			PG_RETURN_TEXT_P(cstring_to_text("Unsure how to plot data set."));
	}
	
	/* Apply GUC variables to respective gnuplot commands */
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

	appendStringInfoString(&gnuplot_script_buf, plot_statement.data);
	
	f = fopen(gnuplot_script_filename.data, "w");
	
	if (f == NULL) {
	  PG_RETURN_TEXT_P(cstring_to_text("Unable to write temporary gnuplot script file in /tmp"));
	}
	
	fprintf(f, "%s", gnuplot_script_buf.data);
	fclose(f);
	
	/* 
	 * Execute Gnuplot script.  
	 * 
	 * Call the gnuplot binary to run the gnuplot script generated above.
	 *
	 */
	ret = system(gnuplot_command.data);
	if (ret < 0) {
		PG_RETURN_TEXT_P(cstring_to_text("There was an error running the gnuplot command"));
	}
	
	/* 
	 * Read and return the output from running the generated gnuplot script
	 */
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


/*
 *   Make a determination on how to plot the result set based on some rigid 
 * heuristics. The rules are based on the number of rows in the result set
 * and the data type of the first and second columns.
 *
 *  1.  Number of records in result set
 *
 *  single row         ->  Horizontal Stacked Bar
 *  
 *  2.  Values of first column / Second column
 *
 *  timestamp    / ordinal       ->  Timeseries
 *  timestamptz  / ordinal       ->  Timeseries
 *
 *  text         / ordinal       ->  Histogram
 *  
 *  numeric      / ordinal       ->  Scatter
 *  int          / ordinal       ->  Scatter
 *  ... ordinals / ordinal       ->  Scatter
 *
 *
 *    If doesn't match, then give up or just try whatever.  Remember, users
 *  are always free to override heuristic based commands by supplying their
 *  own commands to plot, or even using the more manual gnuplot() function.
 */
int infer_chart_type(int n_records, char* first_col_field_type, char* second_col_field_type) {
	
	if (n_records == 1) {
		 return GNUPLOT_HORIZ_BAR;
	} else if (strcmp(first_col_field_type, "timestamp") == 0 || 
						strcmp(first_col_field_type, "timestamptz") == 0
						) {
		return GNUPLOT_TIMESERIES;
		
	} else if (strcmp(first_col_field_type, "text") == 0 && (
							 strcmp(second_col_field_type, "numeric") == 0 ||
							 strcmp(second_col_field_type, "real") == 0 ||
							 strcmp(second_col_field_type, "float") == 0 ||
							 strcmp(second_col_field_type, "bigint") == 0 ||
							 strcmp(second_col_field_type, "int") == 0 ||
							 strcmp(second_col_field_type, "decimal") == 0)
						) {
		return GNUPLOT_HISTOGRAM;
		
	}	else if ((strcmp(first_col_field_type, "numeric") == 0 ||
							 strcmp(first_col_field_type, "real") == 0 ||
							 strcmp(first_col_field_type, "float") == 0 ||
							 strcmp(first_col_field_type, "bigint") == 0 ||
							 strcmp(first_col_field_type, "int") == 0 ||
							 strcmp(first_col_field_type, "decimal") == 0) &&
								 (strcmp(second_col_field_type, "numeric") == 0 ||
								 strcmp(second_col_field_type, "real") == 0 ||
								 strcmp(second_col_field_type, "float") == 0 ||
								 strcmp(second_col_field_type, "bigint") == 0 ||
								 strcmp(second_col_field_type, "int") == 0 ||
								 strcmp(second_col_field_type, "decimal") == 0)
						) {
		return GNUPLOT_SCATTER;
	}
	
	return GNUPLOT_UNKNOWN;
}




