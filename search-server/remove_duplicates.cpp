#include "remove_duplicates.h"

#include <set>
#include <map>

void RemoveDuplicates(SearchServer& search_server)
{
	std::vector<int> documents_to_remove;
	std::map<std::set<std::string>, int> first_unique_documents;

	for (const int document_id : search_server)
	{
		std::set<std::string> words_in_document;
		for (const auto& [word, _] : search_server.GetWordFrequencies(document_id))
		{
			words_in_document.insert(word);
		}
		if (first_unique_documents.count(words_in_document) == 1)
		{
			documents_to_remove.push_back(document_id);
		}
		else
		{
			first_unique_documents[words_in_document] = document_id;
		}
	}

	for(int document_id : documents_to_remove)
	{
		std::cout << "Found duplicate document id " << document_id << std::endl;
		search_server.RemoveDocument(document_id);
	}
}