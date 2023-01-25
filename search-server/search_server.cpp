#include "search_server.h"
#include <cmath>
#include <algorithm>

SearchServer::SearchServer() = default;

SearchServer::SearchServer(const std::string& stop_words_text)
	: SearchServer(SplitIntoWords(stop_words_text))  // Invoke delegating constructor from string container
{}

void SearchServer::SetStopWords(const std::string& text)
{
	for (const std::string& word : SplitIntoWords(text))
	{
		stop_words_.insert(word);
	}
}

void SearchServer::AddDocument(int document_id, const std::string& document,
	DocumentStatus status, const std::vector<int>& ratings)
{
	if (documents_.count(document_id) == 1 || document_id < 0)
	{
		throw std::invalid_argument("Document with this id already exists or id less then 0");
	}

	const std::vector<std::string> words = SplitIntoWordsNoStop(document);

	if (!std::all_of(words.begin(), words.end(), [this](const std::string& word)
		{
			return IsValidWord(word);
		}))
	{
		throw std::invalid_argument("One or more words contain a special symbol");
	}

	const double inv_word_count = 1.0 / words.size();
	std::set<std::string> words_set;
	for (const std::string& word : words)
	{
		words_set.insert(word);
		word_to_document_freqs_[word][document_id] += inv_word_count;
		document_to_word_freqs_[document_id][word] += inv_word_count;
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

//int SearchServer::GetDocumentId(int index) const
//{
//	return documents_id_in_adding_order_.at(index);
//}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const
{
	return FindTopDocuments(raw_query, [](const int document_id, DocumentStatus status, int rating)
		{
			return status == DocumentStatus::ACTUAL;
		});
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const
{
	//LOG_DURATION_STREAM(("Матчинг документов по запросу: " + raw_query), std::cout);

	const Query query = ParseQuery(raw_query);

	std::vector<std::string> words;

	for (const std::string& minus_word : query.minus_words)
	{
		if (word_to_document_freqs_.count(minus_word) == 1)
		{
			if (word_to_document_freqs_.at(minus_word).count(document_id) == 1)
			{
				return std::tuple(words, documents_.at(document_id).status);
			}
		}
	}

	for (const std::string& plus_word : query.plus_words)
	{
		if (word_to_document_freqs_.count(plus_word) == 1)
		{
			if (word_to_document_freqs_.at(plus_word).count(document_id) == 1)
			{
				words.push_back(plus_word);
			}
		}
	}

	return std::tuple(words, documents_.at(document_id).status);
}

bool SearchServer::IsStopWord(const std::string& word) const
{
	return stop_words_.count(word) > 0;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const
{
	std::vector<std::string> words;
	for (const std::string& word : SplitIntoWords(text))
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string text) const
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

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const
{
	Query query;
	const std::vector<std::string> words(SplitIntoWords(text));

	if (!std::all_of(words.begin(), words.end(), [this](const std::string& word)
		{
			return IsValidWord(word);
		}))
	{
		throw std::invalid_argument("One or more words contain a special symbol");
	}

	for (const std::string& word : words)
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

double SearchServer::ComputeWordInverseDocumentFreq(const std::string& word) const
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

bool SearchServer::IsValidWord(const std::string& word) const
{
	return none_of(word.begin(), word.end(), [](char c)
		{
			return c >= 0 && c <= 31;
		});
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
	static std::map<std::string, double> word_freqs;
	const auto result = document_to_word_freqs_.find(document_id);
	if(result == document_to_word_freqs_.end())
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
	for(const auto& [word, _] : words_in_document)
	{
		word_to_document_freqs_[word].erase(document_id);
	}
	document_to_word_freqs_.erase(document_id);
}