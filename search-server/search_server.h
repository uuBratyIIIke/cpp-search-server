#pragma once
#include <string>
#include <vector>
#include <map>
#include <tuple>
#include <numeric>
#include <algorithm>
#include <set>
#include <stdexcept>
#include "document.h"
#include "string_processing.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer
{
public:

	SearchServer();

	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);

	explicit SearchServer(const std::string& stop_words_text);

	void SetStopWords(const std::string& text);

	void AddDocument(int document_id, const std::string& document,
		DocumentStatus status, const std::vector<int>& ratings);

	int GetDocumentCount() const;

	int GetDocumentId(int index) const;
	
	std::vector<Document> FindTopDocuments(const std::string& raw_query) const;

	template <typename Criterion>
	std::vector<Document> FindTopDocuments(const std::string& raw_query, Criterion criterion) const;

	std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

private:

	struct DocumentInfo
	{
		int rating;
		DocumentStatus status;
	};

	std::map<int, DocumentInfo> documents_;

	std::vector <int> documents_id_in_adding_order;

	std::map<std::string, std::map<int, double>> word_to_document_freqs_;

	std::set<std::string> stop_words_;

	bool IsStopWord(const std::string& word) const;

	bool IsValidWord(const std::string& word) const;

	std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	struct QueryWord
	{
		std::string data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(std::string text) const;

	struct Query
	{
		std::set<std::string> plus_words;
		std::set<std::string> minus_words;
	};

	Query ParseQuery(const std::string& text) const;

	double ComputeWordInverseDocumentFreq(const std::string& word) const;

	std::vector<Document> FindAllDocuments(const Query& query, DocumentStatus status) const;

	template <typename Criterion>
	std::vector<Document> FindAllDocuments(const Query& query, Criterion criterion) const;
};


template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words)
	: stop_words_(MakeUniqueNonEmptyStrings(stop_words))
{
	if (!std::all_of(stop_words_.begin(), stop_words_.end(), [this](const std::string& word)
		{
			return IsValidWord(word);
		}))
	{
		throw std::invalid_argument("Ono or more stop words contain a special symbol");
	}
}

template <typename Criterion>
std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, Criterion criterion) const
{
	const Query query = ParseQuery(raw_query);

	auto matched_documents = FindAllDocuments(query, criterion);

	sort(matched_documents.begin(), matched_documents.end(),
		[](const Document& lhs, const Document& rhs)
		{
			return lhs > rhs;
		});

	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
	{
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}

	return matched_documents;
}

template <typename Criterion>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Criterion criterion) const
{
	std::map<int, double> document_to_relevance;
	for (const std::string& word : query.plus_words)
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

	for (const std::string& word : query.minus_words)
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

	std::vector<Document> matched_documents;
	for (const auto [document_id, relevance] : document_to_relevance)
	{
		matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
	}
	return matched_documents;
}