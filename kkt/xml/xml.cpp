/* Разбор XML для ККТ. (c) gsr 2018 */

#include <string.h>
#include "kkt/xml.h"
#include "rapidxml.hpp"

using namespace rapidxml;

bool parse_kkt_xml(char *data, bool check, kkt_xml_callback_t cbk, char **ep, const char **emsg)
{
	bool ret = false;
	xml_document<> xml;
	try {
		xml.parse<parse_validate_closing_tags | parse_trim_whitespace>(data);
		*ep = NULL;
		*emsg = NULL;
		ret = true;
	} catch (const parse_error &pe){
		*ep = pe.where<char>();
		*emsg = strdup(pe.what());
	}
	return ret;
}
