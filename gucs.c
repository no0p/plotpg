#include "plotpg.h"

/* 
* Sets a bunch of GUCs.	Called at so load.
*
*	Includes gucs for a subset of gnuplot's set commands.
*
*/
void _PG_init(void) {

	/* Output mode for plots.	svg or dumb.	Review how to make the so load at server start. */
	DefineCustomStringVariable("plotpg.terminal", "gnuplot terminals 'svg', 'dumb' supported",
                              NULL, &gnuplot_terminal, "dumb", PGC_USERSET, 0,
                              NULL, NULL, NULL);
                              
	DefineCustomStringVariable("plotpg.title",
                              "Plot title",
                              NULL,
                              &gnuplot_title,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);

	DefineCustomStringVariable("plotpg.xlabel",
                              "label for the xaxis",
                              NULL,
                              &gnuplot_xlabel,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);

	DefineCustomStringVariable("plotpg.ylabel",
                              "label for the y axis",
                              NULL,
                              &gnuplot_ylabel,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);
                              
	DefineCustomStringVariable("plotpg.xrange",
                              "range for x-axis",
                              NULL,
                              &gnuplot_xrange,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);
                              
	DefineCustomStringVariable("plotpg.yrange",
                              "range for y axis",
                              NULL,
                              &gnuplot_yrange,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);
                              
	DefineCustomStringVariable("plotpg.xtics",
                              "gnuplot xtics setting",
                              NULL,
                              &gnuplot_xtics,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);             
                              
	DefineCustomStringVariable("plotpg.ytics",
                              "gnuplot ytics setting",
                              NULL,
                              &gnuplot_ytics,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL); 
                              
                              
	DefineCustomStringVariable("plotpg.key",
                              "gnuplot key setting",
                              NULL,
                              &gnuplot_key,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);  

	DefineCustomStringVariable("plotpg.border",
                              "gnuplot border setting",
                              NULL,
                              &gnuplot_border,
                              "",
                              PGC_USERSET,
                              0,
                              NULL, NULL, NULL);
                              
                              
  // A debug option                            
	DefineCustomIntVariable("plotpg.persist",
                            "plotpg will leave gnuplot script and output files in /tmp when set to 1.",
                            NULL,
                            &plotpg_persist,
                            1,
                            0,
                            INT_MAX,
                            PGC_SIGHUP,
                            0,
                            NULL,
                            NULL,
                            NULL);  
                                                                     

}
