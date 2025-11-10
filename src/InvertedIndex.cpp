#include "InvertedIndex.h"
#include <sstream>
#include <algorithm>
#include <thread>
#include <vector>

void InvertedIndex::UpdateDocumentBase(std::vector<std::string> input_docs) {
    docs = std::move(input_docs);
    freq_dictionary.clear();
    
    std::vector<std::thread> threads;
    for (size_t i = 0; i < docs.size(); ++i) {
        threads.emplace_back(&InvertedIndex::indexDocument, this, i, std::ref(docs[i]));
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
}

void InvertedIndex::indexDocument(size_t doc_id, const std::string& text) {
    std::stringstream ss(text);
    std::string word;
    std::map<std::string, size_t> wordCount;
    
    while (ss >> word) {
        if (word.length() > 100) continue;
        ++wordCount[word];
    }
    
    std::lock_guard<std::mutex> lock(dict_mutex);
    for (const auto& [word, count] : wordCount) {
        auto& entries = freq_dictionary[word];
        auto it = std::find_if(entries.begin(), entries.end(),
            [doc_id](const Entry& entry) { return entry.doc_id == doc_id; });
        
        if (it != entries.end()) {
            it->count = count;
        } else {
            entries.push_back({doc_id, count});
        }
    }
}

std::vector<Entry> InvertedIndex::GetWordCount(const std::string& word) {
    std::lock_guard<std::mutex> lock(dict_mutex);
    auto it = freq_dictionary.find(word);
    if (it != freq_dictionary.end()) {
        return it->second;
    }
    return {};
}