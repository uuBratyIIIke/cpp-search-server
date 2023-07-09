#pragma once
#include <set>
#include <string>
#include <vector>
#include <stdexcept>
#include <string_view>

std::vector<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& strings)
{
	std::set<std::string, std::less<>> non_empty_strings;
	for (const auto str : strings)
	{
		if (!str.empty())
		{
			non_empty_strings.insert(str);
		}
	}
	return non_empty_strings;
}