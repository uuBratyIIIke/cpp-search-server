#include "remove_duplicates.h"

#include <set>

void RemoveDuplicates(SearchServer& search_server)
{
	std::vector<int> documents_to_remove;

	for(const auto& [_, document_ids]: search_server.GetWordsSetToDocument())
	{
		for(const auto& document_id : document_ids)
		{
			if(*document_ids.begin() != document_id)
			{
				documents_to_remove.push_back(document_id);
			}
		}
	}

	for(int document_id : documents_to_remove)
	{
		std::cout << "Found duplicate document id " << document_id << std::endl;
		search_server.RemoveDocument(document_id);
	}
}
