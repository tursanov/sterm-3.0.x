#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "rapidxml.hpp"

using namespace rapidxml;

static bool test_xml(const char *xml)
{
	bool ret = false;
	char *buf = strdup(xml);
	xml_document<> parser;
	try {
		parser.parse<parse_validate_closing_tags | parse_trim_whitespace>(buf);
		printf("XML parsed\n");
		ret = true;
	} catch (const parse_error &pe){
		fprintf(stderr, "%s: %s\n", pe.what(), pe.where<char>());
	}
	free(buf);
	return ret;
}

static const char *xml =
	"<K O=\"1\" S=\"20104251234724\" P=\"0100088574\" "
	"H=\"+79271111111\" M=\"2\" T=\"+79272634444\" "
	"E=\"ALEX.P.POPOV@GMAIL.COM\">\n\r"
	"<L S=\"tarif\" P=\"1\" R=\"1\" T=\"6313.8\" N=\"5\"/>\n\r"
	"<L S=\"serwis\" P=\"1\" R=\"1\" T=\"307.4\" N=\"5\"/></K>\n\r";

int main()
{
	test_xml(xml);
	return 0;
}
