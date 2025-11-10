#include "SearchServer.h"
#include <sstream>
#include <algorithm>
#include <map>
#include <cmath>

std::vector<std::vector<RelativeIndex>> SearchServer::search(const std::vector<std::string>& queries_input) {
    std::vector<std::vector<RelativeIndex>> result;

    for (const auto& query : queries_input) {
        result.push_back(processQuery(query));
    }

    return result;
}

std::vector<RelativeIndex> SearchServer::processQuery(const std::string& query) {
    std::stringstream ss(query);
    std::string word;
    std::vector<std::string> words;

    // Разбиваем запрос на слова
    while (ss >> word) {
        if (word.length() > 100) continue;
        words.push_back(word);
    }

    if (words.empty()) {
        return {};
    }

    // Собираем все уникальные документы, содержащие хотя бы одно слово
    std::map<size_t, float> docRelevance;

    // Для каждого слова в запросе
    for (const auto& word : words) {
        auto wordEntries = _index->GetWordCount(word);

        // Для каждого документа, содержащего это слово
        for (const auto& entry : wordEntries) {
            // Суммируем количество вхождений этого слова
            docRelevance[entry.doc_id] += static_cast<float>(entry.count);
        }
    }

    // Если не нашли ни одного документа, возвращаем пустой результат
    if (docRelevance.empty()) {
        return {};
    }

    // Находим максимальную релевантность для нормализации
    float maxRelevance = 0.0f;
    for (const auto& [doc_id, relevance] : docRelevance) {
        if (relevance > maxRelevance) {
            maxRelevance = relevance;
        }
    }

    // Формируем результат с нормализованной релевантностью
    std::vector<RelativeIndex> result;
    for (auto& [doc_id, relevance] : docRelevance) {
        float normalizedRank = (maxRelevance > 0) ? (relevance / maxRelevance) : 0;
        // Округляем для избежания проблем с точностью float
        normalizedRank = std::round(normalizedRank * 1000.0f) / 1000.0f;
        result.push_back({doc_id, normalizedRank});
    }

    // Сортируем по убыванию релевантности
    std::sort(result.begin(), result.end(),
              [](const RelativeIndex& a, const RelativeIndex& b) {
                  if (std::abs(a.rank - b.rank) < 0.0001f) {
                      return a.doc_id < b.doc_id; // при равной релевантности по ID
                  }
                  return a.rank > b.rank;
              });

    return result;
}