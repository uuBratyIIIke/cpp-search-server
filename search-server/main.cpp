#include <algorithm>
#include <iomanip>
#include <cmath>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

struct Document
{
	int id;
	double relevance;
	int rating;
};

ostream& operator<<(ostream& os, const Document& doc)
{
	return os << "("s << doc.id << ", "s << doc.relevance << ", "s << doc.rating << ")"s;
}

bool operator==(const Document doc1, const Document doc2)
{
	return doc1.id == doc2.id && doc1.rating == doc2.rating && abs(doc1.relevance - doc2.relevance) < EPSILON;
}

template <typename Fst, typename Snd>
ostream& operator<<(ostream* os, const pair <Fst, Snd>& pr)
{
	return os << pr.first << ": "s << pr.second;
}

template <typename Container>
void Print(ostream& os, const Container& container)
{
	bool is_first = true;
	for (const auto& element : container)
	{
		if (!is_first)
		{
			os << ", "s;
		}
		os << element;
		is_first = false;
	}
}

template <typename T>
ostream& operator<<(ostream& os, const vector<T>& v)
{
	os << "["s;
	Print(os, v);
	return os << "]"s;
}

template <typename T>
ostream& operator<<(ostream& os, const set<T>& st)
{
	os << "{"s;
	Print(os, st);
	return os << "}"s;
}

template <typename Key, typename Value>
ostream& operator<<(ostream& os, const map<Key, Value>& mp)
{
	os << "{"s;
	Print(os, mp);
	return os << "}"s;
}

string ReadLine()
{
	string s;
	getline(cin, s);
	return s;
}

int ReadLineWithNumber()
{
	int result;
	cin >> result;
	ReadLine();
	return result;
}

vector<string> SplitIntoWords(const string& text)
{
	vector<string> words;
	string word;
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

enum class DocumentStatus
{
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED
};

class SearchServer
{
public:


	void SetStopWords(const string& text)
	{
		for (const string& word : SplitIntoWords(text))
		{
			stop_words_.insert(word);
		}
	}

	void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings)
	{
		const vector<string> words = SplitIntoWordsNoStop(document);
		const double inv_word_count = 1.0 / words.size();
		for (const string& word : words)
		{
			word_to_document_freqs_[word][document_id] += inv_word_count;
		}
		documents_.emplace(document_id, DocumentInfo{ ComputeAverageRating(ratings), status });
	}

	int GetDocumentCount() const
	{
		return static_cast<int>(documents_.size());
	}

	vector<Document> FindTopDocuments(const string& raw_query) const
	{
		return FindTopDocuments(raw_query, [](const int document_id, DocumentStatus status, int rating)
			{
				return status == DocumentStatus::ACTUAL;
			});
	}

	template <typename Criterion>
	vector<Document> FindTopDocuments(const string& raw_query, Criterion criterion) const
	{
		const Query query = ParseQuery(raw_query);
		auto matched_documents = FindAllDocuments(query, criterion);

		sort(matched_documents.begin(), matched_documents.end(),
			[](const Document& lhs, const Document& rhs)
			{
				return lhs.relevance > rhs.relevance ||
				(abs(lhs.relevance - rhs.relevance) < EPSILON &&
					lhs.rating > rhs.rating);
			});

		if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
		{
			matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
		}

		return matched_documents;
	}

	tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
	{
		Query query = ParseQuery(raw_query);

		vector <string> words;

		for (const string& minus_word : query.minus_words)
		{
			if (word_to_document_freqs_.count(minus_word) == 1)
			{
				if (word_to_document_freqs_.at(minus_word).count(document_id) == 1)
				{
					return tuple(words, documents_.at(document_id).status);
				}
			}
		}

		for (const string& plus_word : query.plus_words)
		{
			if (word_to_document_freqs_.count(plus_word) == 1)
			{
				if (word_to_document_freqs_.at(plus_word).count(document_id) == 1)
				{
					words.push_back(plus_word);
				}
			}
		}

		return tuple(words, documents_.at(document_id).status);
	}

private:

	struct DocumentInfo
	{
		int rating;
		DocumentStatus status;
	};

	map<int, DocumentInfo> documents_;

	map<string, map<int, double>> word_to_document_freqs_;

	set<string> stop_words_;

	bool IsStopWord(const string& word) const
	{
		return stop_words_.count(word) > 0;
	}

	vector<string> SplitIntoWordsNoStop(const string& text) const
	{
		vector<string> words;
		for (const string& word : SplitIntoWords(text))
		{
			if (!IsStopWord(word))
			{
				words.push_back(word);
			}
		}
		return words;
	}

	static int ComputeAverageRating(const vector<int>& ratings)
	{
		if (ratings.size() == 0)
		{
			return 0;
		}

		return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
	}

	struct QueryWord
	{
		string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(string text) const
	{
		bool is_minus = false;
		if (text[0] == '-')
		{
			is_minus = true;
			text = text.substr(1);
		}
		return
		{
			text,
			is_minus,
			IsStopWord(text)
		};
	}

	struct Query
	{
		set<string> plus_words;
		set<string> minus_words;
	};

	Query ParseQuery(const string& text) const
	{
		Query query;
		for (const string& word : SplitIntoWords(text))
		{
			const QueryWord query_word = ParseQueryWord(word);
			if (!query_word.is_stop)
			{
				if (query_word.is_minus)
				{
					query.minus_words.insert(query_word.data);
				}
				else
				{
					query.plus_words.insert(query_word.data);
				}
			}
		}
		return query;
	}

	double ComputeWordInverseDocumentFreq(const string& word) const
	{
		return log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
	}

	vector<Document> FindAllDocuments(const Query& query, DocumentStatus status) const
	{
		return FindAllDocuments(query, [status](const int document_id, DocumentStatus document_status, const int rating)
			{
				return status == document_status;
			});
	}

	template <typename Criterion>
	vector<Document> FindAllDocuments(const Query& query, Criterion criterion) const
	{
		map<int, double> document_to_relevance;
		for (const string& word : query.plus_words)
		{
			if (word_to_document_freqs_.count(word) == 0)
			{
				continue;
			}
			const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
			for (const auto [document_id, term_freq] : word_to_document_freqs_.at(word))
			{
				if (criterion(document_id, documents_.at(document_id).status, documents_.at(document_id).rating))
				{
					document_to_relevance[document_id] += term_freq * inverse_document_freq;
				}
			}
		}

		for (const string& word : query.minus_words)
		{
			if (word_to_document_freqs_.count(word) == 0)
			{
				continue;
			}
			for (const auto [document_id, _] : word_to_document_freqs_.at(word))
			{
				document_to_relevance.erase(document_id);
			}
		}

		vector<Document> matched_documents;
		for (const auto [document_id, relevance] : document_to_relevance)
		{
			matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
		}
		return matched_documents;
	}
};

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file, const string& func, unsigned line, const string& hint)
{
	if (t != u)
	{
		cerr << boolalpha;
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		cerr << t << " != "s << u << "."s;
		if (!hint.empty())
		{
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))


void AssertImpl(bool value, const string& raw_value, const int line, const string& file, const string& func, const string& hint)
{
	if (!value)
	{
		cerr << noboolalpha;
		cerr << file << "("s << line << "): "s << func << ": "s;
		cerr << "ASSERT("s << raw_value << ") failed."s;
		if (!hint.empty())
		{
			cerr << " Hint: "s << hint;
		}
		cerr << endl;
		abort();
	}
}

#define ASSERT(expr) AssertImpl((expr), #expr, __LINE__, __FILE__, __FUNCTION__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl((expr), #expr, __LINE__, __FILE__, __FUNCTION__, hint)

template <typename Func>
void RunTestImpl(Func func, const string& raw_func)
{
	func();
	cerr << raw_func << " OK" << endl;
}

#define RUN_TEST(func)  RunTestImpl(func, #func)

void TestExcludeStopWordsFromAddedDocumentContent()
{
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = { 1, 2, 3 };
	{
		SearchServer server;
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto found_docs = server.FindTopDocuments("in"s);
		ASSERT_EQUAL(found_docs.size(), 1);
		const Document& doc0 = found_docs[0];
		ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
		SearchServer server;
		server.SetStopWords("in the"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT(server.FindTopDocuments("in"s).empty());
	}
}


void TestAddingNewDocumentToSearchServer()
{
	const int doc_id = 13;
	const string content = "the most loneliest day of my life"s;
	const vector<int> ratings = { 1, 2, 3, 4, 5 };
	{
		SearchServer server;
		server.SetStopWords("the of"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_EQUAL(server.FindTopDocuments("loneliest day"s).size(), 1);
		ASSERT_EQUAL(server.FindTopDocuments("cat"s).size(), 0);
	}
}

void TestExcludingDocumentsWithMinusWords()
{
	const int doc_id = 13;
	const string content = "the most loneliest day of my life"s;
	const vector<int> ratings = { 1, 2, 3, 4, 5 };
	{
		SearchServer server;
		server.SetStopWords("the of"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT(server.FindTopDocuments("loneliest -day"s).empty());
		ASSERT_EQUAL(server.FindTopDocuments("day"s).size(), 1);
	}
}

void TestMatchingDocumentsWithQuery()
{
	const int doc_id = 13;
	const string content = "the most loneliest day of my life"s;
	const vector<int> ratings = { 1, 2, 3, 4, 5 };
	{
		SearchServer server;
		server.SetStopWords("the of"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		vector<string> matched_words = { "loneliest"s, "most"s };
		ASSERT(server.MatchDocument("the most loneliest week"s, 13) == tuple(matched_words, DocumentStatus::ACTUAL));
		matched_words.clear();
		ASSERT(server.MatchDocument("the most -loneliest week"s, 13) == tuple(matched_words, DocumentStatus::ACTUAL));
	}
}

void TestSortingDocuments()
{
	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
	const vector <Document> sorted_documents_from_server = search_server.FindTopDocuments("пушистый ухоженный кот"s);
	const vector<Document> correctly_sorted_documents = {
		{ 1, 0.866434, 5 },
		{ 0, 0.173287, 2 },
		{ 2, 0.173287, -1 }
	};

	ASSERT_EQUAL(sorted_documents_from_server, correctly_sorted_documents);
}

void TestDocumentRatingComputing()
{
	const int doc_id = 13;
	const string content = "the most loneliest day of my life"s;
	const vector<int> ratings = { 1, 2, 3, 4, 5 };
	int correct_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
	{
		SearchServer server;
		server.SetStopWords("the of"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_EQUAL(server.FindTopDocuments("most"s).size(), 1);
		ASSERT_EQUAL(server.FindTopDocuments("most"s).front().rating, correct_rating);
	}
}

void TestFindingDocumentsWithUserPredicate()
{
	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });
	const vector<Document> correct_documents_with_user_predicate = {
		{ 0, 0.173287, 2 },
		{ 2, 0.173287, -1 }
	};
	const vector<Document> found_documents_withs_user_predicate = search_server.FindTopDocuments("пушистый ухоженный кот"s, 
														[](int document_id, DocumentStatus status, int rating)
														{
															return document_id % 2 == 0;
														});
	ASSERT_EQUAL(found_documents_withs_user_predicate, correct_documents_with_user_predicate);
}

void TestFindingDocumentsWithUserDocumentStatus()
{
	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

	const vector<Document> correct_documents_with_user_documents_status = { { 3, 0.231049, 9 } };
	const vector<Document> found_documents_with_user_documents_status = 
		search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED);

	ASSERT_EQUAL(search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED), 
				correct_documents_with_user_documents_status);
}

void TestTfIdfComputing()
{
	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

	ASSERT(!search_server.FindTopDocuments("пушистый ухоженный кот"s).empty());
	ASSERT(abs(search_server.FindTopDocuments("пушистый ухоженный кот"s).front().relevance - 0.866434) < 1e-6);
}

void TestSearchServer()
{
	RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
	RUN_TEST(TestAddingNewDocumentToSearchServer);
	RUN_TEST(TestMatchingDocumentsWithQuery);
	RUN_TEST(TestExcludingDocumentsWithMinusWords);
	RUN_TEST(TestDocumentRatingComputing);
	RUN_TEST(TestSortingDocuments);
	RUN_TEST(TestTfIdfComputing);
	RUN_TEST(TestFindingDocumentsWithUserPredicate);
	RUN_TEST(TestFindingDocumentsWithUserDocumentStatus);
}

int main()
{
	TestSearchServer();
	cout << "Search server testing finished"s << endl;
	return 0;
}