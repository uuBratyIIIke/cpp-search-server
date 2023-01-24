#include "string_processing.h"
#include <algorithm>

std::vector<std::string> SplitIntoWords(const std::string& text)
{
	std::vector<std::string> words;
	std::string word;
	for (const char c : text)
	{
		if (c == ' ')
		{
			words.push_back(word);
			word = "";
		}
		else
		{
			word += c;
		}
	}
	words.push_back(word);

	return words;
}