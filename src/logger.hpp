#pragma once
#include <fstream>
#include <string>


enum class log_level {
    NONE, TRACE
};

class logger {

    std::ofstream logs;
    std::ofstream results;
    log_level level = log_level::NONE;

public:
    logger() : logs("logs.txt"), results("results.txt") {  }
    logger( const std::string &logs, const std::string &res ) : logs( logs )
                                                              , results( res ) { }
    void set_log_level( log_level newlev ) {
        level = newlev;
    }

    bool enabled() const {
        return level != log_level::NONE;
    }


    std::ofstream& log() {
        return logs;
    }

    std::ofstream& logresult() {
        return results;
    }
};
