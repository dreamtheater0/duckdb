
#include "parser/parser.hpp"

#include "parser/transformer.hpp"

using namespace postgres;

using namespace duckdb;
using namespace std;

Parser::Parser()  {
}

void Parser::ParseQuery(std::string query) {
	void *context = nullptr;
	PgQueryInternalParsetreeAndError result;
    // first try to parse any PRAGMA statements
    if (ParsePragma(query)) {
        // query parsed as pragma statement
        // if there was no error we were successful
        goto wrapup;
    }

    // use the postgres parser to parse the query
    context = pg_query_parse_init();
    result = pg_query_parse(query.c_str());
    // check if it succeeded
    if (result.error) {
        throw ParserException(string(result.error->message) + "[" +
                        to_string(result.error->lineno) + ":" +
                        to_string(result.error->cursorpos) + "]");
        goto wrapup;
    }

    if (!result.tree) {
        // empty statement
        goto wrapup;
    }

    // if it succeeded, we transform the Postgres parse tree into a list of
    // SQLStatements
    Transformer transformer;
    if (!transformer.TransformParseTree(result.tree, statements)) {
        goto wrapup;
    }
wrapup:
	if (context) {
		pg_query_parse_finish(context);
		pg_query_free_parse_result(result);
	}
}

enum class PragmaType : uint8_t { NOTHING, ASSIGNMENT, CALL };

bool Parser::ParsePragma(std::string &query) {
	// check if there is a PRAGMA statement, this is done before calling the
	// postgres parser
	static const string pragma_string = "PRAGMA";
	auto query_cstr = query.c_str();

	// skip any spaces
	size_t pos = 0;
	while (isspace(query_cstr[pos]))
		pos++;

	if (pos + pragma_string.size() >= query.size()) {
		// query is too small, can't contain PRAGMA
		return false;
	}

	if (query.compare(pos, pragma_string.size(), pragma_string.c_str()) != 0) {
		// statement does not start with PRAGMA
		return false;
	}
	pos += pragma_string.size();
	// string starts with PRAGMA, parse the pragma
	// first skip any spaces
	while (isspace(query_cstr[pos]))
		pos++;
	// now look for the keyword
	size_t keyword_start = pos;
	while (query_cstr[pos] && query_cstr[pos] != ';' &&
	       query_cstr[pos] != '=' && query_cstr[pos] != '(' &&
	       !isspace(query_cstr[pos]))
		pos++;

	// no keyword found
	if (pos == keyword_start) {
		throw ParserException("Invalid PRAGMA: PRAGMA without keyword");
	}

	string keyword = query.substr(keyword_start, pos - keyword_start);

	while (isspace(query_cstr[pos]))
		pos++;

	PragmaType type;
	if (query_cstr[pos] == '=') {
		// assignment
		type = PragmaType::ASSIGNMENT;
	} else if (query_cstr[pos] == '(') {
		// function call
		type = PragmaType::CALL;
	} else {
		// nothing
		type = PragmaType::NOTHING;
	}

	if (keyword == "table_info") {
		if (type != PragmaType::CALL) {
			throw ParserException(
			    "Invalid PRAGMA table_info: expected table name");
		}
		ParseQuery("SELECT * FROM pragma_" + query.substr(keyword_start));
	} else {
		throw ParserException("Unrecognized PRAGMA keyword: %s",
		                      keyword.c_str());
	}

	return true;
}
