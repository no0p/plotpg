-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use "CREATE EXTENSION plotpg" to load this file. \quit

CREATE FUNCTION sine()
RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C IMMUTABLE STRICT;


CREATE FUNCTION plot(text, text DEFAULT '')
RETURNS text
AS 'MODULE_PATHNAME'
LANGUAGE C VOLATILE;



--CREATE FUNCTION plot_no_file(text, text DEFAULT '')
--RETURNS text
--AS 'MODULE_PATHNAME'
--LANGUAGE C;


-- functions on column names

-- quartiles
-- pie
-- bar charts
