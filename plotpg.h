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

PG_MODULE_MAGIC;

void _PG_init(void);
Datum	sine(void);
Datum plot(PG_FUNCTION_ARGS);
Datum plot_no_file(PG_FUNCTION_ARGS);

char* gnuplot_terminal;
char* gnuplot_title;
char* gnuplot_xlabel;
char* gnuplot_ylabel;
