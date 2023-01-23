#include "string_processing.h"
#include <algorithm>

bool IsValidWord(const std::string& word)
{
	return none_of(word.begin(), word.end(), [](char c)
		{
			return c >= 0 && c <= 31;
		});
}

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
	if (!all_of(words.begin(), words.end(), IsValidWord))
	{
		throw std::invalid_argument("One or more stop words contain a special symbol");
	}
	return words;
}