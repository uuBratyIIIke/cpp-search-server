#pragma once
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <tuple>
#include <numeric>
#include <algorithm>
#include <set>
#include <execution>
#include <stdexcept>

#include "document.h"
#include "string_processing.h"
#include "log_duration.h"
#include "concurrent_map.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer
{
public:

	SearchServer();

	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);
	explicit SearchServer(const std::string& stop_words_text);
	explicit SearchServer(const std::string_view stop_words_text);

	std::set<int>::const_iterator begin() const;

	std::set<int>::iterator  begin();

	std::set<int>::const_iterator end() const;

	std::set<int>::iterator end();

	void SetStopWords(const std::string& text);

	void AddDocument(int document_id, const std::string_view document,
		DocumentStatus status, const std::vector<int>& ratings);

	int GetDocumentCount() const;

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy& policy, const std::string_view raw_query) const;
	std::vector<Document> FindTopDocuments(const std::string_view raw_query) const;
	template <typename ExecutionPolicy, typename Criterion>
	std::vector<Document> FindTopDocuments(ExecutionPolicy& policy, const std::string_view raw_query, Criterion criterion) const;
	template <typename Criterion>
	std::vector<Document> FindTopDocuments(const std::string_view raw_query, Criterion criterion) const;

	using MatchDocumentType = std::tuple<std::vector<std::string_view>, DocumentStatus>;

	MatchDocumentType MatchDocument(const std::string_view raw_query, int document_id) const;
	MatchDocumentType MatchDocument(const std::execution::sequenced_policy&,const std::string_view raw_query, int document_id) const;
	MatchDocumentType MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const;

	void RemoveDocument(int document_id);
	void RemoveDocument(const std::execution::sequenced_policy&, int document_id);
	void RemoveDocument(const std::execution::parallel_policy&, int document_id);

private:

	struct DocumentInfo
	{
		int rating;
		DocumentStatus status;
	};

	std::map<int, DocumentInfo> documents_;
	std::set<int> document_ids_;
	std::map<std::string_view, std::map<int, double>> word_to_document_freqs_;
	std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
	std::set<std::string, std::less<>> stop_words_;
	std::set<std::string, std::less<>> all_words_;

	bool IsStopWord(const std::string_view word) const;

	bool IsValidWord(const std::string_view word) const;

	std::vector<std::string_view> SplitIntoWordsNoStop(const std::string_view text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);

	struct QueryWord
	{
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	QueryWord ParseQueryWord(std::string_view text) const;

	struct Query
	{
		std::vector<std::string_view> plus_words;
		std::vector<std::string_view> minus_words;
	};

	Query ParseQuery(const std::string_view text, bool sort_needed = true) const;

	double ComputeWordInverseDocumentFreq(const std::string_view word) const;

	std::vector<Document> FindAllDocuments(const Query& query, DocumentStatus status) const;

	template <typename Criterion>
	std::vector<Document> FindAllDocuments(const Query& query, Criterion criterion) const;
	template <typename Criterion>
	std::vector<Document> FindAllDocuments(const std::execution::parallel_policy&, const Query& query, Criterion criterion) const;
	template <typename Criterion>
	std::vector<Document> FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, Criterion criterion) const;
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
		throw std::invalid_argument("One or more stop words contain a special symbol");
	}
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy& policy, const std::string_view raw_query) const
{
	return FindTopDocuments(policy, raw_query, [](const int document_id, DocumentStatus status, int rating)
		{
			return status == DocumentStatus::ACTUAL;
		});
}

template <typename ExecutionPolicy, typename Criterion>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy& policy, const std::string_view raw_query, Criterion criterion) const
{
	//LOG_DURATION_STREAM(("Результаты поиска по запросу: " + raw_query), std::cout);
	const Query query = ParseQuery(raw_query, true);

	auto matched_documents = FindAllDocuments(policy, query, criterion);

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
std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query, Criterion criterion) const
{
	return FindTopDocuments(std::execution::seq, raw_query, criterion);
}

template <typename Criterion>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query, Criterion criterion) const
{
	std::map<int, double> document_to_relevance;
	for (const auto& word : query.plus_words)
	{
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
		for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word))
		{
			const auto& document_id_data = documents_.at(document_id);
			if (criterion(document_id, document_id_data.status, document_id_data.rating)) 
			{
				document_to_relevance[document_id] += term_freq * inverse_document_freq;
			}
		}
	}

	for (const auto& word : query.minus_words)
	{
		if (word_to_document_freqs_.count(word) == 0)
		{
			continue;
		}
		for (const auto& [document_id, _] : word_to_document_freqs_.at(word))
		{
			document_to_relevance.erase(document_id);
		}
	}

	std::vector<Document> matched_documents;
	for (const auto& [document_id, relevance] : document_to_relevance)
	{
		matched_documents.push_back({ document_id, relevance, documents_.at(document_id).rating });
	}
	return matched_documents;
}

template <typename Criterion>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::sequenced_policy&, const Query& query, Criterion criterion) const
{
	return FindAllDocuments(query, criterion);
}

template<typename Criterion>
std::vector<Document> SearchServer::FindAllDocuments(const std::execution::parallel_policy&, const Query& query, Criterion criterion) const
{
	ConcurrentMap<int, double> document_to_relevance(std::max(static_cast<int>(query.plus_words.size()), 100));

	std::for_each(std::execution::par, query.plus_words.begin(), query.plus_words.end(), [&document_to_relevance, this, &criterion](const auto& word) 
		{
			if (word_to_document_freqs_.count(word) != 0) 
			{
				const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
				for (const auto& [document_id, term_freq] : word_to_document_freqs_.at(word)) 
				{
					const auto& document_id_data = documents_.at(document_id);
					if (criterion(document_id, document_id_data.status, document_id_data.rating)) 
					{
						document_to_relevance[document_id].ref_to_value += term_freq * inverse_document_freq;
					}
				}
			}
		});


	std::for_each(std::execution::par, query.minus_words.begin(), query.minus_words.end(), 
		[&document_to_relevance, this](const auto& word) 
		{
			if (word_to_document_freqs_.count(word) != 0) 
			{
				for (const auto [document_id, _] : word_to_document_freqs_.at(word)) 
				{
					document_to_relevance.Erase(document_id);
				}
			}
		});

	std::vector<Document> matched_documents;
	std::map<int, double> result = document_to_relevance.BuildOrdinaryMap();
	matched_documents.reserve(result.size());

	std::for_each(std::execution::par, result.begin(), result.end(), 
		[&matched_documents, this](const auto& doc) 
		{
			matched_documents.push_back({ doc.first, doc.second, documents_.at(doc.first).rating });
		});

	return matched_documents;
}