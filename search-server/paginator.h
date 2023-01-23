#pragma once
#include <vector>

template <typename RandomIt>
class IteratorRange
{
public:

	IteratorRange(RandomIt begin, RandomIt end)
		:begin_(begin)
		, end_(end)
	{}

	RandomIt begin()
	{
		return begin_;
	}

	RandomIt end()
	{
		return end_;
	}

	RandomIt begin() const
	{
		return begin_;
	}

	RandomIt end() const
	{
		return end_;
	}

	size_t size() const
	{
		return distance(begin_, end_);
	}

private:

	RandomIt begin_, end_;
};

template <typename RandomIt>
std::ostream& operator<<(std::ostream& os, const IteratorRange<RandomIt>& iter_range)
{
	for (const auto element : iter_range)
	{
		os << element;
	}
	return os;
}

template <typename Iter>
class Paginator
{
public:

	Paginator(Iter range_begin, Iter range_end, size_t page_size)
	{
		int num_pages = (range_end - range_begin) / page_size;
		if ((range_end - range_begin) % page_size != 0)
		{
			++num_pages;
		}
		for (int i = 0; i < num_pages; ++i)
		{
			auto fst_record = next(range_begin, page_size * i);
			auto lst_record = next(fst_record, std::min(page_size, static_cast<size_t>(range_end - fst_record)));
			pages_.push_back({ fst_record, lst_record });
		}
	}

	auto begin()
	{
		return pages_.begin();
	}

	auto end()
	{
		return pages_.end();
	}

	auto begin() const
	{
		return pages_.begin();
	}

	auto end() const
	{
		return pages_.end();
	}

	size_t size() const
	{
		return pages_.size();
	}

private:

	std::vector<IteratorRange<Iter>> pages_;
};

template <typename Container>
auto Paginate(const Container& container, size_t page_size)
{
	return Paginator(container.begin(), container.end(), page_size);
}
