-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION plotpg" to load this file. \quit

CREATE FUNCTION plot(text, text DEFAULT '')
RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C VOLATILE;

CREATE FUNCTION gnuplot(text)
RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C VOLATILE;


-- functions on column names

-- quartiles
-- pie
-- bar charts
