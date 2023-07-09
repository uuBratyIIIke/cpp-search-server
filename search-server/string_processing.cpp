#include "string_processing.h"
#include <algorithm>


std::vector<std::string_view> SplitIntoWords(std::string_view text)
{
	std::vector<std::string_view> result;
	text.remove_prefix(std::min(text.find_first_not_of(" "), text.size()));
	const int64_t pos_end = text.npos;

	while (!text.empty()) {
		int64_t space = text.find(' ');
		result.push_back(text.substr(0, space));
		text.remove_prefix(space == pos_end ? text.size() : space);
		text.remove_prefix(std::min(text.find_first_not_of(" "), text.size()));
	}
	return result;
}