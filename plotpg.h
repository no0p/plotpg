#include "postgres.h"
#include "fmgr.h"
#include "utils/builtins.h"
#include "executor/spi.h"
#include "lib/stringinfo.h"
#include "tcop/utility.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>

void _PG_init(void);
Datum	sine(void);
Datum plot(PG_FUNCTION_ARGS);

/* variables for gnuplot-esque gucs */
char* gnuplot_terminal;
char* gnuplot_title;
char* gnuplot_xlabel;
char* gnuplot_ylabel;
char* gnuplot_xrange;
char* gnuplot_yrange;
