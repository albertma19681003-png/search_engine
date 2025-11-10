#pragma once

#include "InvertedIndex.h"
#include <vector>
#include <string>
#include <algorithm>
#include <memory>

struct RelativeIndex {
    size_t doc_id;
    float rank;

    bool operator ==(const RelativeIndex& other) const {
        return (doc_id == other.doc_id && rank == other.rank);
    }
};

class SearchServer {
public:
    SearchServer(std::shared_ptr<InvertedIndex> idx) : _index(idx) { };

    std::vector<std::vector<RelativeIndex>> search(const std::vector<std::string>& queries_input);

private:
    std::shared_ptr<InvertedIndex> _index;

    std::vector<RelativeIndex> processQuery(const std::string& query);
};