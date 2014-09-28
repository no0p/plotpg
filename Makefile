MODULE_big= plotpg
OBJS=plotpg.o sine.o gucs.o

EXTENSION = plotpg
DATA = plotpg--1.0.sql

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
