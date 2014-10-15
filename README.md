## Introduction

Plotpg is an open source experiment to generate charts from postgresql result sets.

The <a href="http://no0p.github.io/plotpg/">plotpg project page</a> provides a *demo* of basic features.

More technical details and examples are available in the <a href="https://github.com/no0p/plotpg/wiki">project wiki</a>.

## Installation

Plotpg is a typical postgresql extension.

```
git clone git@github.com:no0p/plotpg.git
cd plotpg
make
sudo make install
```

Plotpg provides a <a href="https://github.com/no0p/plotpg/wiki/Plotting-Data">plot</a> function for plotting data, and a <a href="https://github.com/no0p/plotpg/wiki/Plotting-Functions">gnuplot</a> function for plotting analytic functions.  After installation, these functions can be added to a database with the following command in psql.

```
=# create extension plotpg;
```

### A Gnuplot Dependency

Plotpg uses the *gnuplot* program to generate plot data.  So the extension will only work if this application is installed.  On an ubuntu system the following command will suffice:

```
sudo apt-get install gnuplot gnuplot-x11
```

## Basic Usage

Please see the <a href="https://github.com/no0p/plotpg/wiki">project wiki</a> for usage examples.  A basic pattern of use follows:

```
select plot('select price, weight from widgets');
```


