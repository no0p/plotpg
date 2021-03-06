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

#define GNUPLOT_UNKNOWN   -1
#define GNUPLOT_HISTOGRAM  1
#define GNUPLOT_HORIZ_BAR  2
#define GNUPLOT_TIMESERIES 3
#define GNUPLOT_SCATTER    4

void _PG_init(void);
Datum plot(PG_FUNCTION_ARGS);
Datum gnuplot(PG_FUNCTION_ARGS);

int infer_chart_type(int, char*, char*);
bool is_ordinal(char*);

/* variables for gnuplot-esque gucs */
char* gnuplot_terminal;
char* gnuplot_size;
char* gnuplot_title;
char* gnuplot_xlabel;
char* gnuplot_ylabel;
char* gnuplot_xrange;
char* gnuplot_yrange;
char* gnuplot_xtics;
char* gnuplot_ytics;
char* gnuplot_key;
char* gnuplot_border;

int plotpg_persist;
