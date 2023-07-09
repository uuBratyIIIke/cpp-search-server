#include "search_server.h"
#include <cmath>
#include <algorithm>

SearchServer::SearchServer() = default;

SearchServer::SearchServer(const std::string& stop_words_text)
	: SearchServer(std::string_view(stop_words_text))
{}


SearchServer::SearchServer(const std::string_view stop_words_text)
{
	if (!IsValidWord(stop_words_text)) {
		throw std::invalid_argument("stop words contain invalid characters");
	}
	for (auto word : SplitIntoWords(stop_words_text)) {
		stop_words_.insert(std::string(word));
	}
}

void SearchServer::SetStopWords(const std::string& text)
{
	for (const std::string_view word : SplitIntoWords(std::string_view(text)))
	{
		stop_words_.insert(word.data());
	}
}

void SearchServer::AddDocument(int document_id, std::string_view document,
	DocumentStatus status, const std::vector<int>& ratings)
{
	if (documents_.count(document_id) == 1 || document_id < 0)
	{
		throw std::invalid_argument("Document with this id already exists or id less then 0");
	}

	const std::vector<std::string_view> words = SplitIntoWordsNoStop(document);

	if (!std::all_of(words.begin(), words.end(), [this](const auto word)
		{
			return IsValidWord(word);
		}))
	{
		throw std::invalid_argument("One or more words contain a special symbol");
	}

	const double inv_word_count = 1.0 / words.size();
	for (const auto word : words)
	{
		auto it = all_words_.emplace(std::string(word));
		word_to_document_freqs_[*(it.first)][document_id] += inv_word_count;
		document_to_word_freqs_[document_id][*(it.first)] += inv_word_count;
	}
	documents_.emplace(document_id, DocumentInfo{ ComputeAverageRating(ratings), status });
	document_ids_.insert(document_id);
}

int SearchServer::GetDocumentCount() const
{
	return static_cast<int>(documents_.size());
}

std::set<int>::const_iterator SearchServer::begin() const
{
	return document_ids_.begin();
}

std::set<int>::iterator SearchServer::begin()
{
	return document_ids_.begin();
}

std::set<int>::const_iterator SearchServer::end() const
{
	return document_ids_.end();
}

std::set<int>::iterator SearchServer::end()
{
	return document_ids_.end();
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string_view raw_query) const
{
	return FindTopDocuments(raw_query, [](const int document_id, DocumentStatus status, int rating)
		{
			return status == DocumentStatus::ACTUAL;
		});
}

using MatchDocumentType = std::tuple<std::vector<std::string_view>, DocumentStatus>;

MatchDocumentType SearchServer::MatchDocument(const std::string_view raw_query, int document_id) const
{
	//LOG_DURATION_STREAM(("Матчинг документов по запросу: " + raw_query), std::cout);

	const Query query = ParseQuery(raw_query, true);

	std::vector<std::string_view> words;

	if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
			[this, &document_id](const auto word)
			{
				return document_to_word_freqs_.at(document_id).count(word) != 0;
			})
		)
	{
		return { words, documents_.at(document_id).status };
	}

	for (const auto plus_word : query.plus_words)
	{
		if (word_to_document_freqs_.count(plus_word) == 1)
		{
			if (word_to_document_freqs_.at(plus_word).count(document_id))
			{
				words.push_back(plus_word);
			}
		}
	}

	return{ words, documents_.at(document_id).status };
}

MatchDocumentType SearchServer::MatchDocument(const std::execution::sequenced_policy&, const std::string_view raw_query, int document_id) const
{
	return MatchDocument(raw_query, document_id);
}

MatchDocumentType SearchServer::MatchDocument(const std::execution::parallel_policy&, const std::string_view raw_query, int document_id) const
{
	if (document_ids_.count(document_id) == 0) 
	{
		throw std::out_of_range("the document id does not exist");
	}

	const Query query = ParseQuery(raw_query, false);

	MatchDocumentType result { {}, documents_.at(document_id).status };

	if (std::any_of(std::execution::par, query.minus_words.begin(), query.minus_words.end(),
			[this, &document_id](const auto word)
			{
				return document_to_word_freqs_.at(document_id).count(word) != 0;
			})
		)
	{
		return result;
	}

	std::get<0>(result).resize(query.plus_words.size());

	auto it_end = std::copy_if(std::execution::par, query.plus_words.begin(), query.plus_words.end(),
		std::get<0>(result).begin(), [this, &document_id](const auto word)
		{
			return word_to_document_freqs_.count(word) && word_to_document_freqs_.at(word).count(document_id);
		});
	std::sort(std::get<0>(result).begin(), it_end);
	it_end = std::unique(std::get<0>(result).begin(), it_end);
	std::get<0>(result).erase(it_end, std::get<0>(result).end());

	return result;
}


bool SearchServer::IsStopWord(const std::string_view word) const
{
	return stop_words_.count(word) > 0;
}

std::vector<std::string_view> SearchServer::SplitIntoWordsNoStop(const std::string_view text) const
{
	std::vector<std::string_view> words;
	for (const std::string_view word : SplitIntoWords(text))
	{
		if (!IsStopWord(word))
		{
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings)
{
	if (ratings.size() == 0)
	{
		return 0;
	}

	return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const
{
	bool is_minus = false;
	if (text[0] == '-')
	{
		if (text.size() == 1 || text[1] == '-')
		{
			throw std::invalid_argument("Invalid minus word in query");
		}
		is_minus = true;
		text = text.substr(1);
	}
	return { text,is_minus,IsStopWord(text) };
}

SearchServer::Query SearchServer::ParseQuery(const std::string_view text, bool sort_needed) const
{
	Query query;
	const std::vector<std::string_view> words(SplitIntoWords(text));

	if (!std::all_of(words.begin(), words.end(), [this](const auto word)
			{
				return IsValidWord(word);
			})
		)
	{
		throw std::invalid_argument("One or more words contain a special symbol");
	}

	for (const std::string_view word : words)
	{
		const QueryWord query_word = ParseQueryWord(word);

		if (!query_word.is_stop)
		{
			if (query_word.is_minus)
			{
				query.minus_words.push_back(query_word.data);
			}
			else
			{
				query.plus_words.push_back(query_word.data);
			}
		}
	}

	if(sort_needed)
	{
		for (auto* word : { &query.plus_words, &query.minus_words }) 
		{
			std::sort(word->begin(), word->end());
			word->erase(unique(word->begin(), word->end()), word->end());
		}
	}
	
	return query;
}

double SearchServer::ComputeWordInverseDocumentFreq(const std::string_view word) const
{
	return std::log(documents_.size() * 1.0 / word_to_document_freqs_.at(word).size());
}

std::vector<Document> SearchServer::FindAllDocuments(const Query& query, DocumentStatus status) const
{
	return FindAllDocuments(query, [status](const int document_id, DocumentStatus document_status, const int rating)
		{
			return status == document_status;
		});
}

bool SearchServer::IsValidWord(const std::string_view word) const
{
	return std::none_of(word.begin(), word.end(), [](char c)
		{
			return c >= 0 && c <= 31;
		});
}

const std::map<std::string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	static std::map<std::string_view, double> word_freqs;
	const auto result = document_to_word_freqs_.find(document_id);
	if (result == document_to_word_freqs_.end())
	{
		return word_freqs;
	}

	return result->second;
}

void SearchServer::RemoveDocument(int document_id)
{
	documents_.erase(document_id);
	document_ids_.erase(document_id);
	const auto& words_in_document = GetWordFrequencies(document_id);
	for (const auto& [word, _] : words_in_document)
	{
		word_to_document_freqs_[word].erase(document_id);
	}
	document_to_word_freqs_.erase(document_id);
}

void SearchServer::RemoveDocument(const std::execution::sequenced_policy&, int document_id)
{
	RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(const std::execution::parallel_policy&, int document_id)
{
	if(document_ids_.count(document_id) == 0)
	{
		return;
	}

	auto& word_freq = document_to_word_freqs_.at(document_id);
	std::vector<std::string_view> words(word_freq.size());
	std::transform(std::execution::par, word_freq.begin(), word_freq.end(),
		words.begin(),
		[](const auto& word) { return word.first; });
	std::for_each(std::execution::par, words.begin(), words.end(),
		[this, document_id](const auto& word) {
			word_to_document_freqs_[word].erase(document_id);
		});
	documents_.erase(document_id);
	document_ids_.erase(document_id);
	document_to_word_freqs_.erase(document_id);
}