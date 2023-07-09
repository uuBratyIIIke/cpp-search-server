#pragma once
#include <algorithm>
#include <cstdlib>
#include <future>
#include <map>
#include <string>
#include <vector>
#include <mutex>

#include "log_duration.h"


using namespace std::string_literals;

template <typename Key, typename Value>
class ConcurrentMap
{
private:

    struct Bucket
    {
        std::mutex bucket_mutex;
        std::map<Key, Value> bucket_map;
    };

public:
    static_assert(std::is_integral_v<Key>, "ConcurrentMap supports only integer keys"s);

    struct Access
    {
        std::lock_guard<std::mutex> guard;
        Value& ref_to_value;

        Access(const Key& key, Bucket& bucket)
            :guard(bucket.bucket_mutex)
            , ref_to_value(bucket.bucket_map[key])
        {}
    };

    explicit ConcurrentMap(size_t bucket_count) :buckets_(bucket_count)
    {}

    Access operator[](const Key& key)
    {
        auto& bucket = buckets_[static_cast<uint64_t>(key) % buckets_.size()];
        return { key, bucket };
    }

    std::map<Key, Value> BuildOrdinaryMap()
    {
        std::map<Key, Value> result;
        for (auto& [mutex, map] : buckets_)
        {
            std::lock_guard guard(mutex);
            result.insert(map.begin(), map.end());
        }

        return result;
    }

	void Erase(const Key& key)
	{
        auto index = static_cast<uint64_t>(key) % buckets_.size();
        {
            std::lock_guard guard(buckets_[index].bucket_mutex);
            buckets_[index].bucket_map.erase(key);
        }
    }

private:

    std::vector<Bucket> buckets_;
};