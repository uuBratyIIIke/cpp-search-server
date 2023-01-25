#pragma once

#include <chrono>
#include <iostream>

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION(x) LogDuration UNIQUE_VAR_NAME_PROFILE(x)

#define PROFILE_CONCAT_INTERNAL(X, Y) X ## Y
#define PROFILE_CONCAT(X, Y) PROFILE_CONCAT_INTERNAL(X, Y)
#define UNIQUE_VAR_NAME_PROFILE PROFILE_CONCAT(profileGuard, __LINE__)
#define LOG_DURATION_STREAM(x, y) LogDuration UNIQUE_VAR_NAME_PROFILE(x, y)

class LogDuration
{
public:

    using Clock = std::chrono::steady_clock;

    LogDuration(const std::string& name, std::ostream& output = std::cerr)
        :name_(name)
        ,output_(output)
    {
        /*if (output_.rdbuf() == std::cout.rdbuf())
        {
            output_ << name_ << std::endl;
        }*/
    }

    ~LogDuration()
    {
        using namespace std::chrono;
        using namespace std::literals;

        const auto end_time = Clock::now();
        const auto dur = end_time - start_time_;
        if (output_.rdbuf() == std::cerr.rdbuf())
        {
            output_ << name_ << ": "s;
        }
        else
        {
            output_ << "Operation time: "s;
        }
        output_ << duration_cast<milliseconds>(dur).count() << " ms"s << std::endl;
    }

private:

    const Clock::time_point start_time_ = Clock::now();
    const std::string name_;
    std::ostream& output_;
};