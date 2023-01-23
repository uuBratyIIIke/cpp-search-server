#include <algorithm>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <numeric>

#include "request_queue.h"
#include "search_server.h"
#include "test_example_functions.h"
#include "document.h"
#include "paginator.h"

using namespace std;

void PrintDocument(const Document& document)
{
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s << endl;
}

int main()
{
	setlocale(LC_ALL, "RUS");
	TestSearchServer();
	cout << "Search server testing finished"s << endl;


	SearchServer search_server("and in at"s);
	RequestQueue request_queue(search_server);
	search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
	search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
	search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
	search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
	search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
	// 1439 запросов с нулевым результатом
	for (int i = 0; i < 1439; ++i) 
	{
		request_queue.AddFindRequest("empty request"s);
	}
	// все еще 1439 запросов с нулевым результатом
	request_queue.AddFindRequest("curly dog"s);
	// новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
	request_queue.AddFindRequest("big collar"s);
	// первый запрос удален, 1437 запросов с нулевым результатом
	request_queue.AddFindRequest("sparrow"s);
	cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
	return 0;
}