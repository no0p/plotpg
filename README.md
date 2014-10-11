## Introduction

Plotpg is an experiment to generate charts from postgresql result sets.

The <a href="http://no0p.github.io/plotpg/">plotpg project page</a> provides a *demo* and an introduction to *basic usage*.

More technical details and examples are available in the <a href="https://github.com/no0p/plotpg/wiki">project wiki</a>.

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


