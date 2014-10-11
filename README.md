## Introduction

Plotpg is an experiment to generate charts from postgresql result sets.

It provides functions which interact with gnuplot to generate the charts.

<a href="http://no0p.github.io/plotpg/">Project Page</a>

## Installation

Plotpg is a typical postgresql extension.

```
git clone git@github.com:no0p/plotpg.git
cd plotpg
make
sudo make install
```

### A Gnuplot Dependency

Plotpg uses the *gnuplot* program to generate plot data.  So the extension will only work if this application is installed, and the gnuplot binary is in the path of the postgres user.  On an ubuntu system the following command will suffice:

```
sudo apt-get install gnuplot gnuplot-x11
```

## Usage

In psql load the extension.  Then test whether it installed correctly by calling the sine() function:

```
psql=# create extension plotpg;
CREATE EXTENSION

psql=# select sine();
                                     sine                                      
-------------------------------------------------------------------------------
                                                                              +
     1 ++---------------***---------------+---**-----------+--------**-----++ +
       +                *  *              +  *  **         +  sin(x) ****** + +
   0.8 ++              *   *                 *    *               *    *   ++ +
       |              *     *               *     *               *     *   | +
   0.6 *+             *      *              *     *               *     *  ++ +
       |*             *      *             *       *             *       *  | +
   0.4 +*            *       *             *       *             *       * ++ +
       |*            *        *            *        *           *        *  | +
   0.2 +*           *         *            *        *           *         *++ +
     0 ++*          *          *          *         *          *          *++ +
       | *          *          *         *           *         *           *| +
  -0.2 ++ *         *          *         *           *         *           *+ +
       |  *        *           *        *             *        *           *| +
  -0.4 ++ *        *            *       *             *       *            *+ +
       |  *       *              *      *             *      *              * +
  -0.6 ++  *      *              *      *             *      *             +* +
       |    *    *               *     *               *     *              | +
  -0.8 ++   *    *                *   *                 *   *              ++ +
       +     *  *       +         **  *   +             *  *                + +
    -1 ++-----**--------+-----------**----+--------------***---------------++ +
      -10              -5                 0                5                10+
                                                                              +
 
(1 row)

```

### Plotting data

Data is plotted by passing an sql query to the plot() function.  For example we'll plot a straight line for an exemplar "activities" table which has a surrogate key.

```
=# select count(*) from activities;
 count 
-------
 73478
(1 row)

=# select plot('select id, id from activities');
                                      plot                                      
--------------------------------------------------------------------------------
                                                                               +
   80000 ++------+--------+-------+--------+-------+-------+--------+------++  +
         +       +        +  '/tmp/plotpg_data_10187.data' using 1:2+  A    +  +
   70000 ++                                                        AAAAA   ++  +
         |                                                      AAAA        |  +
         |                                                  AAAAA           |  +
   60000 ++                                              AAAA              ++  +
         |                                           AAAAA                  |  +
   50000 ++                                       AAAA                     ++  +
         |                                    AAAAA                         |  +
   40000 ++                                AAAA                            ++  +
         |                             AAAA                                 |  +
         |                         AAAAA                                    |  +
   30000 ++                     AAAA                                       ++  +
         |                  AAAAA                                           |  +
   20000 ++              AAAA                                              ++  +
         |           AAAAA                                                  |  +
         |        AAAA                                                      |  +
   10000 ++   AAAAA                                                        ++  +
         + AAAA  +        +       +        +       +       +        +       +  +
       0 AAA-----+--------+-------+--------+-------+-------+--------+------++  +
         0     10000    20000   30000    40000   50000   60000    70000   80000+
                                                                               +
 
(1 row)
```


Plot accepts two parameters.  The first is the sql query which provides the result set which will be plotted.  The second parameter is optional, and it is a string that will be included in the gnuplot script.  This enables the user to add arbitrary gnuplot commands.  Consider the following example which adds an arrow to the plot above.

```
=# select plot('select id, id from activities limit 10', 'set arrow 1 from 2, 6 to 5, 6; set label 1 "interesting" at 2,7 center;');
                                     plot                                      
-------------------------------------------------------------------------------
                                                                              +
   10 ++------+-------+------+-------+-------+-------+------+-------+------+A +
      +       +       +      '/tmp/plotpg_data_11949.data' using 1:2+  A    + +
    9 ++                                                            A      ++ +
      |                                                                     | +
    8 ++                                                    A              ++ +
      |                                                                     | +
    7 ++ interesting                                 A                     ++ +
      |                                                                     | +
    6 ++      >>>>>>>>>>>>>>>>>>>>>>>>       A                             ++ +
      |                                                                     | +
      |                                                                     | +
    5 ++                             A                                     ++ +
      |                                                                     | +
    4 ++                     A                                             ++ +
      |                                                                     | +
    3 ++              A                                                    ++ +
      |                                                                     | +
    2 ++      A                                                            ++ +
      +       +       +      +       +       +       +      +       +       + +
    1 A+------+-------+------+-------+-------+-------+------+-------+------++ +
      1       2       3      4       5       6       7      8       9       10+
                                                                              +
 
(1 row)
```


### Output modes

Three output modes are the most useful: 'dumb', 'svg', and 'png'.  These are set through a GUC:

```
psql=# set terminal='svg';
SET

=# select plot('select id, id from activities limit 10', 'set arrow 1 from 2, 6 to 5, 6; set label 1 "interesting" at 2,7 center;');
--------------------------------------------------------------------------------------------------------
 <?xml version="1.0" encoding="utf-8"  standalone="no"?>
 <!DOCTYPE svg PUBLIC "-//W3C//DTD SVG 1.1// ...
 <svg width="600" height="480">
   ....
```

After the terminal value is set, calls to plot() during the current session will now output in the selected mode.  In dumb mode, the plot will printed in ASCII text similar to the sine example in the introduction.  In *svg* mode an svg plot will be returned


### Gnuplot Details

Many gnuplot options can be set as postgresql variables.  Similar to the gnuplot interactive mode, these variables are set as follows:

```
set xrange='[0:1000]'
set title='My plot'
```

These variable will be set when the next plot command is issued.  For more information refer to the gnuplot documentation.




