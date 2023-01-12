#include <algorithm>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine()
{
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber()
{
    int result = 0;
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
            if (!word.empty()) 
            {
                words.push_back(word);
                word.clear();
            }
        }
        else 
        {
            word += c;
        }
    }
    if (!word.empty()) 
    {
        words.push_back(word);
    }

    return words;
}

struct Document
{
    int id;
    double relevance;
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

    void AddDocument(int document_id, const string& document)
	{
        const vector <string> words_no_stop = SplitIntoWordsNoStop(document);
        for(const string& word : words_no_stop)
        {
            documents_[word][document_id] += 1. / words_no_stop.size();
        }
        ++document_count_;
    }

    vector<Document> FindTopDocuments(const string& raw_query) const
	{
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) 
            {
                return lhs.relevance > rhs.relevance;
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) 
        {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }
        return matched_documents;
    }

private:

    struct Query
    {
        set <string> plus_words;
        set <string> minus_words;
    };

    map<string, map <int, double>> documents_;

    set<string> stop_words_;

    int document_count_ = 0;

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

    Query ParseQuery(const string& text) const
	{
        Query query_words;
        for (const string& word : SplitIntoWordsNoStop(text)) 
        {
			if(word[0] == '-')
			{
                query_words.minus_words.insert(string(word.begin() + 1, word.end()));
			}
            else
            {
                query_words.plus_words.insert(word);
            }
        }
        return query_words;
    }
    
    vector<Document> FindAllDocuments(const Query& query_words) const
	{
        map <int, double> raw_matched_documents;

        for(const string& plus_word : query_words.plus_words)
        {
            if (documents_.count(plus_word) == 1)
            {
                const double idf = log(1. * document_count_ / documents_.at(plus_word).size());
                for(auto& [id, tf] : documents_.at(plus_word))
                {
                    raw_matched_documents[id] += tf * idf;
                }
            }
        }

        for(const string& minus_word : query_words.minus_words)
        {
            if (documents_.count(minus_word) == 1)
            {
                for (const auto& [id, _] : documents_.at(minus_word))
                {
                    raw_matched_documents.erase(id);
                }
            }
        }

    	vector<Document> matched_documents;

        for(const auto& [id, relevance] : raw_matched_documents)
        {
            matched_documents.push_back({ id, relevance });
        }

        return matched_documents;
    }

};

SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; ++document_id) 
    {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
}

int main()
{
    const SearchServer search_server = CreateSearchServer();
    
    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query)) 
    {
        cout << "{ document_id = "s << document_id << ", "
            << "relevance = "s << relevance << " }"s << endl;
    }
    return 0;
}