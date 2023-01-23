#pragma once
#include <iostream>

const double EPSILON = 1e-6;

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
    };

struct Document
{

    Document();

    Document(int id, double relevance, int rating);

    int id = 0;
    double relevance = 0;
    int rating = 0;
};

std::ostream& operator<<(std::ostream& os, const Document& doc);

bool operator==(const Document doc1, const Document doc2);

bool operator>(const Document doc1, const Document doc2);