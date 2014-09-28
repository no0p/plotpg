#include "plotpg.h"

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
                              NULL, NULL, NULL);

	DefineCustomStringVariable("xlabel",
                              "label for the xaxis",
                              NULL,
                              &gnuplot_xlabel,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);

	DefineCustomStringVariable("ylabel",
                              "label for the y axis",
                              NULL,
                              &gnuplot_ylabel,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);
                              
	DefineCustomStringVariable("xrange",
                              "range for x-axis",
                              NULL,
                              &gnuplot_xrange,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);
                              
	DefineCustomStringVariable("yrange",
                              "range for y axis",
                              NULL,
                              &gnuplot_yrange,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);

}
