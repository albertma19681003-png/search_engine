#include <iostream>
#include <memory>
#include <filesystem>
#include "ConverterJSON.h"
#include "InvertedIndex.h"
#include "SearchServer.h"

void printCurrentDirectory() {
    std::cout << "📁 Current working directory: " << std::filesystem::current_path() << std::endl;
}

void checkRequiredFiles() {
    std::cout << "🔍 Checking required files..." << std::endl;

    std::vector<std::string> filesToCheck = {
        "../resources/config.json",
        "../../resources/config.json", 
        "resources/config.json",
        "../resources/requests.json",
        "../../resources/requests.json",
        "resources/requests.json"
    };

    for (const auto& file : filesToCheck) {
        if (std::filesystem::exists(file)) {
            std::cout << "   ✓ " << file << std::endl;
        } else {
            std::cout << "   ✗ " << file << " (not found)" << std::endl;
        }
    }
}

int main() {
    system("chcp 65001");
    try {
        std::cout << "🚀 === SEARCH ENGINE ===" << std::endl;

        printCurrentDirectory();
        checkRequiredFiles();

        // Инициализация компонентов
        std::cout << "🔄 Initializing search engine..." << std::endl;
        ConverterJSON converter;
        auto index = std::make_shared<InvertedIndex>();
        SearchServer server(index);

        // Получаем лимит ответов из конфигурации
        int maxResponses = converter.GetResponsesLimit();

        // Загрузка и индексация документов
        std::cout << "📚 Loading and indexing documents..." << std::endl;
        auto documents = converter.GetTextDocuments();
        std::cout << "✅ Loaded " << documents.size() << " documents" << std::endl;

        index->UpdateDocumentBase(documents);
        std::cout << "✅ Documents indexed successfully" << std::endl;

        // Получение запросов
        std::cout << "🔎 Loading search requests..." << std::endl;
        auto requests = converter.GetRequests();
        std::cout << "✅ Loaded " << requests.size() << " search requests" << std::endl;

        // Выполнение поиска
        std::cout << "⚡ Processing search queries..." << std::endl;
        auto allSearchResults = server.search(requests);
        std::cout << "✅ Search completed" << std::endl;

        // Подготовка результатов с учетом max_responses
        std::vector<std::vector<std::pair<int, float>>> answers;
        
        for (const auto& result : allSearchResults) {
            std::vector<std::pair<int, float>> queryResult;
            
            // Сохраняем оригинальную сортировку (по убыванию релевантности)
            for (size_t i = 0; i < result.size() && i < static_cast<size_t>(maxResponses); ++i) {
                queryResult.emplace_back(static_cast<int>(result[i].doc_id), result[i].rank);
            }
            
            answers.push_back(queryResult);
        }

        // Выводим статистику
        std::cout << "📋 Results summary:" << std::endl;
        size_t totalResults = 0;
        size_t successfulRequests = 0;
        
        for (size_t i = 0; i < answers.size(); ++i) {
            if (answers[i].empty()) {
                std::cout << "   Request " << (i + 1) << ": ❌ No results" << std::endl;
            } else {
                std::cout << "   Request " << (i + 1) << ": ✅ " << answers[i].size()
                          << " document(s)" << std::endl;
                totalResults += answers[i].size();
                successfulRequests++;
            }
        }
        
        std::cout << "📈 Total successful requests: " << successfulRequests << "/" << answers.size() << std::endl;
        std::cout << "📊 Total documents in results: " << totalResults << std::endl;

        // Сохранение результатов
        std::cout << "💾 Saving results to answers.json..." << std::endl;
        converter.putAnswers(answers);

        std::cout << "🎉 === SEARCH COMPLETED SUCCESSFULLY ===" << std::endl;

    } catch (const std::exception& e) {
        std::cerr << "❌ === ERROR ===" << std::endl;
        std::cerr << "Error: " << e.what() << std::endl;
        printCurrentDirectory();
        return 1;
    }

    return 0;
}