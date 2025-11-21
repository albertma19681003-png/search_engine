#include "SearchServer.h"
#include <sstream>
#include <algorithm>
#include <map>
#include <cmath>
#include <set>
#include <unordered_map>

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

    // Шаг 1: Получаем документы, содержащие каждое слово
    std::vector<std::vector<Entry>> wordEntriesList;
    
    for (const auto& word : words) {
        auto wordEntries = _index->GetWordCount(word);
        if (wordEntries.empty()) {
            // Если хотя бы одно слово не найдено ни в одном документе - возвращаем пустой результат
            return {};
        }
        wordEntriesList.push_back(wordEntries);
    }

    // Шаг 2: Находим документы, содержащие ВСЕ слова (AND логика)
    // Начинаем с первого множества документов
    std::map<size_t, float> docRelevance;
    
    if (!wordEntriesList.empty()) {
        // Используем первый список слов как базовый
        for (const auto& entry : wordEntriesList[0]) {
            docRelevance[entry.doc_id] = static_cast<float>(entry.count);
        }
        
        // Пересекаем с остальными списками
        for (size_t i = 1; i < wordEntriesList.size(); ++i) {
            std::map<size_t, float> tempRelevance;
            
            for (const auto& entry : wordEntriesList[i]) {
                // Если документ есть в текущем результате, добавляем его
                if (docRelevance.find(entry.doc_id) != docRelevance.end()) {
                    tempRelevance[entry.doc_id] = docRelevance[entry.doc_id] + static_cast<float>(entry.count);
                }
            }
            
            docRelevance = std::move(tempRelevance);
            
            // Если после пересечения не осталось документов, возвращаем пустой результат
            if (docRelevance.empty()) {
                return {};
            }
        }
    }

    // Шаг 3: Рассчитываем итоговую релевантность для оставшихся документов
    // (уже суммировали вхождения во время пересечения)
    
    // Если не нашли ни одного документа, возвращаем пустой результат
    if (docRelevance.empty()) {
        return {};
    }

    // Шаг 4: Находим максимальную релевантность для нормализации
    float maxRelevance = 0.0f;
    for (const auto& [doc_id, relevance] : docRelevance) {
        if (relevance > maxRelevance) {
            maxRelevance = relevance;
        }
    }

    // Шаг 5: Формируем результат с нормализованной релевантностью
    std::vector<RelativeIndex> result;
    for (auto& [doc_id, relevance] : docRelevance) {
        float normalizedRank = (maxRelevance > 0) ? (relevance / maxRelevance) : 0;
        // Округляем для избежания проблем с точностью float
        normalizedRank = std::round(normalizedRank * 1000.0f) / 1000.0f;
        result.push_back({doc_id, normalizedRank});
    }

    // Шаг 6: Сортируем по убыванию релевантности (как требует ТЗ)
    std::sort(result.begin(), result.end(),
              [](const RelativeIndex& a, const RelativeIndex& b) {
                  // Сначала сравниваем по rank (убывание)
                  if (std::abs(a.rank - b.rank) > 0.0001f) {
                      return a.rank > b.rank;
                  }
                  // При равной релевантности - по возрастанию doc_id
                  return a.doc_id < b.doc_id;
              });

    return result;
}