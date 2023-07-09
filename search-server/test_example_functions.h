#pragma once
#include <iostream>
#include <iomanip>
#include <string>
#include <vector>
#include <set>
#include <map>

template <typename Fst, typename Snd>
std::ostream& operator<<(std::ostream* os, const std::pair <Fst, Snd>& pr)
{
	return os << pr.first << ": " << pr.second;
}

template <typename Container>
void Print(std::ostream& os, const Container& container)
{
	bool is_first = true;
	for (const auto& element : container)
	{
		if (!is_first)
		{
			os << ", ";
		}
		os << element;
		is_first = false;
	}
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& v)
{
	os << "[";
	Print(os, v);
	return os << "]";
}

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::set<T>& st)
{
	os << "{";
	Print(os, st);
	return os << "}";
}

template <typename Key, typename Value>
std::ostream& operator<<(std::ostream& os, const std::map<Key, Value>& mp)
{
	os << "{";
	Print(os, mp);
	return os << "}";
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str,
	const std::string& file, const std::string& func, unsigned line, const std::string& hint)
{
	if (t != u)
	{
		std::cerr << std::boolalpha;
		std::cerr << file << "(" << line << "): " << func << ": ";
		std::cerr << "ASSERT_EQUAL(" << t_str << ", " << u_str << ") failed: ";
		std::cerr << t << " != " << u << ".";
		if (!hint.empty())
		{
			std::cerr << " Hint: " << hint;
		}
		std::cerr << std::endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, "")

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))


void AssertImpl(bool value, const std::string& raw_value, const int line, const std::string& file,
	const std::string& func, const std::string& hint);

#define ASSERT(expr) AssertImpl((expr), #expr, __LINE__, __FILE__, __FUNCTION__, "")

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __LINE__, __FILE__, __FUNCTION__, hint)

template <typename Func>
void RunTestImpl(Func func, const std::string& raw_func)
{
	func();
	std::cerr << raw_func << " OK" << std::endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)

void TestExcludeStopWordsFromAddedDocumentContent();

void TestAddingNewDocumentToSearchServer();

void TestExcludingDocumentsWithMinusWords();

void TestMatchingDocumentsWithQuery();

void TestSortingDocuments();

void TestDocumentRatingComputing();

void TestFindingDocumentsWithUserPredicate();

void TestFindingDocumentsWithUserDocumentStatus();

void TestTfIdfComputing();

void TestSearchServer();