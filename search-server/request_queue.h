#pragma once
#include <vector>
#include <deque>
#include <string>
#include "search_server.h"
#include "document.h"

class RequestQueue
{
public:

	explicit RequestQueue(const SearchServer& search_server);

	template <typename DocumentPredicate>
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

	std::vector<Document> AddFindRequest(const std::string& raw_query);

	int GetNoResultRequests() const;

private:

	struct QueryResult
	{
		std::vector<Document> documents;
	};

	std::deque<QueryResult> requests_;
	const static int min_in_day_ = 1440;
	int empty_results_count_ = 0;
	const SearchServer& server_;
};


template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate)
{
	const auto result = server_.FindTopDocuments(raw_query, document_predicate);
	requests_.push_back({ result });
	if (result.empty())
	{
		++empty_results_count_;
	}
	if (requests_.size() > min_in_day_)
	{
		if (requests_.front().documents.empty())
		{
			--empty_results_count_;
		}
		requests_.pop_front();
	}
	return result;
}