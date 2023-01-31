import numpy
import datetime
import pandas
import pytest
import duckdb

def TestFile(name):
	import os
	filename = os.path.join(os.path.dirname(os.path.realpath(__file__)),'..','data',name)
	return filename

class TestReadCSV(object):
	def test_using_connection_wrapper(self):
		rel = duckdb.read_csv(TestFile('category.csv'))
		res = rel.fetchone()
		print(res)
		assert res == (1, 'Action', datetime.datetime(2006, 2, 15, 4, 46, 27))

	def test_using_connection_wrapper_with_keyword(self):
		rel = duckdb.read_csv(TestFile('category.csv'), dtype={'category_id': 'string'})
		res = rel.fetchone()
		print(res)
		assert res == ('1', 'Action', datetime.datetime(2006, 2, 15, 4, 46, 27))

	def test_no_options(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'))
		res = rel.fetchone()
		print(res)
		assert res == (1, 'Action', datetime.datetime(2006, 2, 15, 4, 46, 27))

	def test_dtype(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'), dtype={'category_id': 'string'})
		res = rel.fetchone()
		print(res)
		assert res == ('1', 'Action', datetime.datetime(2006, 2, 15, 4, 46, 27))

	def test_dtype_as_list(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'), dtype=['string'])
		res = rel.fetchone()
		print(res)
		assert res == ('1', 'Action', datetime.datetime(2006, 2, 15, 4, 46, 27))

		rel = duckdb_cursor.read_csv(TestFile('category.csv'), dtype=['double'])
		res = rel.fetchone()
		print(res)
		assert res == (1.0, 'Action', datetime.datetime(2006, 2, 15, 4, 46, 27))

	def test_sep(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'), sep=" ")
		res = rel.fetchone()
		print(res)
		assert res == ('1|Action|2006-02-15', datetime.time(4, 46, 27))

	def test_delimiter(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'), delimiter=" ")
		res = rel.fetchone()
		print(res)
		assert res == ('1|Action|2006-02-15', datetime.time(4, 46, 27))

	def test_delimiter_and_sep(self, duckdb_cursor):
		with pytest.raises(duckdb.InvalidInputException, match="read_csv takes either 'delimiter' or 'sep', not both"):
			rel = duckdb_cursor.read_csv(TestFile('category.csv'), delimiter=" ", sep=" ")

	def test_header_true(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'), header=True)
		res = rel.fetchone()
		print(res)
		assert res == (1, 'Action', datetime.datetime(2006, 2, 15, 4, 46, 27))

	@pytest.mark.skip(reason="Issue #6011 needs to be fixed first, header=False doesn't work correctly")
	def test_header_false(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'), header=False)

	def test_na_values(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'), na_values='Action')
		res = rel.fetchone()
		print(res)
		assert res == (1, None, datetime.datetime(2006, 2, 15, 4, 46, 27))

	def test_skiprows(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'), skiprows=1)
		res = rel.fetchone()
		print(res)
		assert res == (1, 'Action', datetime.datetime(2006, 2, 15, 4, 46, 27))

	# We want to detect this at bind time
	def test_compression_wrong(self, duckdb_cursor):
		with pytest.raises(duckdb.Error, match="Input is not a GZIP stream"):
			rel = duckdb_cursor.read_csv(TestFile('category.csv'), compression='gzip')

	def test_quotechar(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('unquote_without_delimiter.csv'), quotechar="")
		res = rel.fetchone()
		print(res)
		assert res == ('"AAA"BB',)

	def test_escapechar(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('quote_escape.csv'), escapechar=";")
		res = rel.limit(1,1).fetchone()
		print(res)
		assert res == ('345', 'TEST6', '"text""2""text"')

	def test_encoding_wrong(self, duckdb_cursor):
		with pytest.raises(duckdb.BinderException, match="Copy is only supported for UTF-8 encoded files, ENCODING 'UTF-8'"):
			rel = duckdb_cursor.read_csv(TestFile('quote_escape.csv'), encoding=";")

	def test_encoding_correct(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('quote_escape.csv'), encoding="UTF-8")
		res = rel.limit(1,1).fetchone()
		print(res)
		assert res == (345, 'TEST6', 'text"2"text')

	def test_parallel_true(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'), parallel=True)
		res = rel.fetchone()
		print(res)
		assert res == (1, 'Action', datetime.datetime(2006, 2, 15, 4, 46, 27))

	def test_parallel_true(self, duckdb_cursor):
		rel = duckdb_cursor.read_csv(TestFile('category.csv'), parallel=False)
		res = rel.fetchone()
		print(res)
		assert res == (1, 'Action', datetime.datetime(2006, 2, 15, 4, 46, 27))
