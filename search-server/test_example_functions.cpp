#include "test_example_functions.h"
#include <tuple>
#include <vector>
#include "document.h"
#include "search_server.h"

using namespace std;

void AssertImpl(bool value, const std::string& raw_value, const int line, const std::string& file,
	const std::string& func, const std::string& hint)
{
	if (!value)
	{
		std::cerr << std::noboolalpha;
		std::cerr << file << "("s << line << "): "s << func << ": "s;
		std::cerr << "ASSERT("s << raw_value << ") failed."s;
		if (!hint.empty())
		{
			std::cerr << " Hint: "s << hint;
		}
		std::cerr << std::endl;
		abort();
	}
}

void TestExcludeStopWordsFromAddedDocumentContent()
{
	const int doc_id = 42;
	const std::string content = "cat in the city"s;
	const std::vector<int> ratings = { 1, 2, 3 };
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
		std::string stop_words_string = "in the"s;
		server.SetStopWords(stop_words_string);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT(server.FindTopDocuments("in"s).empty());
	}
}

void TestAddingNewDocumentToSearchServer()
{
	const int doc_id = 13;
	const std::string content = "the most loneliest day of my life"s;
	const std::vector<int> ratings = { 1, 2, 3, 4, 5 };
	{
		SearchServer server;

		std::string stop_words_string = "the of"s;
		server.SetStopWords(stop_words_string);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		ASSERT_EQUAL(server.FindTopDocuments("loneliest day"s).size(), 1);
		ASSERT_EQUAL(server.FindTopDocuments("cat"s).size(), 0);
	}
}

void TestExcludingDocumentsWithMinusWords()
{
	const int doc_id = 13;
	const std::string content = "the most loneliest day of my life"s;
	const std::vector<int> ratings = { 1, 2, 3, 4, 5 };
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
	const std::string content = "the most loneliest day of my life"s;
	const std::vector<int> ratings = { 1, 2, 3, 4, 5 };
	{
		SearchServer server;
		server.SetStopWords("the of"s);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		std::vector<std::string_view> matched_words = { "loneliest"s, "most"s };
		ASSERT(server.MatchDocument("the most loneliest week"s, 13) == std::tuple(matched_words, DocumentStatus::ACTUAL));
		matched_words.clear();
		ASSERT(server.MatchDocument("the most -loneliest week"s, 13) == std::tuple(matched_words, DocumentStatus::ACTUAL));
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
	const std::vector <Document> sorted_documents_from_server = search_server.FindTopDocuments("пушистый ухоженный кот"s);
	const std::vector<Document> correctly_sorted_documents = {
		{ 1, 0.866434, 5 },
		{ 0, 0.173287, 2 },
		{ 2, 0.173287, -1 }
	};

	ASSERT_EQUAL(sorted_documents_from_server, correctly_sorted_documents);
}

void TestDocumentRatingComputing()
{
	const int doc_id = 13;
	const std::string content = "the most loneliest day of my life"s;
	const std::vector<int> ratings = { 1, 2, 3, 4, 5 };
	int correct_rating = accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
	{
		SearchServer server;
		std::string stop_words_string = "the of";
		server.SetStopWords(stop_words_string);
		server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
		const auto result = server.FindTopDocuments("most");
		ASSERT_EQUAL(result.size(), 1);
		ASSERT_EQUAL(result.front().rating, correct_rating);
	}
}

void TestFindingDocumentsWithUserPredicate()
{
	SearchServer search_server;
	search_server.SetStopWords("и в на"s);
	search_server.AddDocument(0, "белый кот и модный ошейник", DocumentStatus::ACTUAL, { 8, -3 });
	search_server.AddDocument(1, "пушистый кот пушистый хвост", DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "ухоженный пёс выразительные глаза", DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
	search_server.AddDocument(3, "ухоженный скворец евгений", DocumentStatus::BANNED, { 9 });
	const std::vector<Document> correct_documents_with_user_predicate = {
		{ 0, 0.173287, 2 },
		{ 2, 0.173287, -1 }
	};
	const std::vector<Document> found_documents_withs_user_predicate = search_server.FindTopDocuments("пушистый ухоженный кот"s,
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

	const std::vector<Document> correct_documents_with_user_documents_status = { { 3, 0.231049, 9 } };
	const std::vector<Document> found_documents_with_user_documents_status =
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

	const auto result = search_server.FindTopDocuments("пушистый ухоженный кот"s);

	ASSERT(!result.empty());
	ASSERT(abs(result.front().relevance - 0.866434) < 1e-6);
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