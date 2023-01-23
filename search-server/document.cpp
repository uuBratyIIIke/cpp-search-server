#include "document.h"

Document::Document() = default;

Document::Document(int id, double relevance, int rating)
	:id(id),
	relevance(relevance),
	rating(rating)
{}

std::ostream& operator<<(std::ostream& os, const Document& doc)
{
	return os << "{ document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating << " }";
}

bool operator==(const Document doc1, const Document doc2)
{
	return doc1.id == doc2.id && doc1.rating == doc2.rating && abs(doc1.relevance - doc2.relevance) < EPSILON;
}

bool operator>(const Document doc1, const Document doc2)
{
	return doc1.relevance > doc2.relevance ||
		(std::abs(doc1.relevance - doc2.relevance) < EPSILON &&
			doc1.rating > doc2.rating);
}