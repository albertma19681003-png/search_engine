#include "ConverterJSON.h"
#include <sstream>
#include <algorithm>
#include <filesystem>

ConverterJSON::ConverterJSON() {
    readConfig();
}

void ConverterJSON::readConfig() {
    // Пробуем несколько относительных путей
    std::vector<std::string> possibleConfigPaths = {
            "../resources/config.json",      // из папки src/
            "../../resources/config.json",   // из папки cmake-build-debug/bin/
            "resources/config.json",         // из корня проекта
            "../config.json"                 // альтернативный путь
    };

    std::ifstream configFile;
    std::string foundPath;

    for (const auto& path : possibleConfigPaths) {
        configFile.open(path);
        if (configFile.is_open()) {
            foundPath = path;
            std::cout << "✓ Found config file at: " << path << std::endl;
            break;
        }
    }

    if (!configFile.is_open()) {
        throw std::runtime_error("config file is missing. Tried paths:\n../resources/config.json\n../../resources/config.json\nresources/config.json");
    }

    // Читаем содержимое файла
    std::string content((std::istreambuf_iterator<char>(configFile)),
                        std::istreambuf_iterator<char>());
    configFile.close();

    // ПРОВЕРКА: файл не должен быть пустым
    if (content.empty()) {
        throw std::runtime_error("config file is empty");
    }

    // ПРОВЕРКА: должен содержать поле "config"
    if (content.find("\"config\"") == std::string::npos) {
        throw std::runtime_error("config file is empty: missing 'config' section");
    }

    // ПРОВЕРКА: должен содержать поле "files"
    if (content.find("\"files\"") == std::string::npos) {
        throw std::runtime_error("config file is empty: missing 'files' section");
    }

    // Парсим базовые значения
    engineName = "ITBoxSearchEngine";
    version = "3.23";
    maxResponses = 5;
    files.clear();

    // Парсим файлы из config.json
    std::istringstream contentStream(content);
    std::string line;  // ДОБАВЛЕНО: объявление переменной line
    bool inFilesArray = false;
    bool foundFiles = false;

    while (std::getline(contentStream, line)) {
        // Ищем начало массива files
        if (line.find("\"files\"") != std::string::npos &&
            line.find("[") != std::string::npos) {
            inFilesArray = true;
            foundFiles = true;
            continue;
        }

        if (inFilesArray) {
            // Ищем пути к файлам
            if (line.find("\"") != std::string::npos) {
                size_t start = line.find("\"") + 1;
                size_t end = line.find("\"", start);
                if (end != std::string::npos) {
                    std::string filePath = line.substr(start, end - start);

                    // Корректируем путь к файлу
                    std::string correctedPath;
                    if (filePath.find("../") == 0) {
                        correctedPath = filePath; // уже относительный путь
                    } else if (filePath.find("resources/") == 0) {
                        correctedPath = "../" + filePath; // добавляем ../
                    } else {
                        correctedPath = "../resources/" + filePath; // полный относительный путь
                    }

                    files.push_back(correctedPath);
                }
            }

            // Конец массива
            if (line.find("]") != std::string::npos) {
                break;
            }
        }
    }

    // ПРОВЕРКА: должен быть хотя бы один файл для индексации
    if (files.empty()) {
        throw std::runtime_error("config file is empty: no files specified in 'files' section");
    }

    // ПРОВЕРКА: поле config должно содержать версию
    if (content.find("\"version\"") == std::string::npos) {
        throw std::runtime_error("config file has incorrect file version: missing 'version' field");
    }

    // Проверяем версию (упрощенная проверка)
    if (content.find("\"version\": \"3.23\"") == std::string::npos) {
        throw std::runtime_error("config.json has incorrect file version. Expected: 3.23");
    }

    std::cout << "Starting " << engineName << " version " << version << std::endl;
    std::cout << "Found " << files.size() << " files to index" << std::endl;
}

std::vector<std::string> ConverterJSON::GetTextDocuments() {
    std::vector<std::string> documents;

    for (size_t i = 0; i < files.size(); ++i) {
        // Пробуем несколько путей для каждого файла
        std::vector<std::string> possibleFilePaths = {
                files[i],                           // оригинальный путь
                "../" + files[i],                   // на уровень выше
                "../../" + files[i],                // на два уровня выше
                "resources/" + files[i].substr(files[i].find_last_of("/") + 1) // только имя файла
        };

        std::ifstream file;
        bool fileOpened = false;

        for (const auto& path : possibleFilePaths) {
            file.open(path);
            if (file.is_open()) {
                std::string content((std::istreambuf_iterator<char>(file)),
                                    std::istreambuf_iterator<char>());
                documents.push_back(content);
                file.close();
                std::cout << "✓ Loaded: " << path << std::endl;
                fileOpened = true;
                break;
            }
        }

        if (!fileOpened) {
            std::cerr << "⚠️  Warning: Cannot open file (tried: " << files[i] << " and variations)" << std::endl;
            documents.push_back(""); // Пустая строка, но приложение продолжает работу
        }
    }

    return documents;
}

int ConverterJSON::GetResponsesLimit() {
    return maxResponses;
}

std::vector<std::string> ConverterJSON::GetRequests() {
    // Пробуем несколько путей для requests.json
    std::vector<std::string> possibleRequestPaths = {
            "../resources/requests.json",      // из папки src/
            "../../resources/requests.json",   // из папки cmake-build-debug/bin/
            "resources/requests.json",         // из корня проекта
            "../requests.json"                 // альтернативный путь
    };

    std::ifstream requestsFile;
    std::string foundPath;

    for (const auto& path : possibleRequestPaths) {
        requestsFile.open(path);
        if (requestsFile.is_open()) {
            foundPath = path;
            std::cout << "✓ Found requests file at: " << path << std::endl;
            break;
        }
    }

    if (!requestsFile.is_open()) {
        throw std::runtime_error("requests.json file is missing");
    }

    std::vector<std::string> requests;
    std::string line;
    std::string content;
    while (std::getline(requestsFile, line)) {
        content += line + "\n";
    }
    requestsFile.close();

    // Парсим запросы из requests.json
    std::istringstream contentStream(content);
    bool inRequestsArray = false;

    while (std::getline(contentStream, line)) {
        // Ищем начало массива requests
        if (line.find("\"requests\"") != std::string::npos &&
            line.find("[") != std::string::npos) {
            inRequestsArray = true;
            continue;
        }

        if (inRequestsArray) {
            // Ищем строки запросов
            if (line.find("\"") != std::string::npos) {
                size_t start = line.find("\"") + 1;
                size_t end = line.find("\"", start);
                if (end != std::string::npos) {
                    std::string request = line.substr(start, end - start);
                    requests.push_back(request);
                }
            }

            // Конец массива
            if (line.find("]") != std::string::npos) {
                break;
            }
        }
    }

    return requests;
}

void ConverterJSON::putAnswers(std::vector<std::vector<std::pair<int, float>>> answers) {
    // Пробуем несколько путей для сохранения answers.json
    std::vector<std::string> possibleAnswerPaths = {
            "../resources/answers.json",      // из папки src/
            "../../resources/answers.json",   // из папки cmake-build-debug/bin/
            "resources/answers.json"          // из корня проекта
    };

    std::ofstream answersFile;
    std::string savedPath;

    for (const auto& path : possibleAnswerPaths) {
        answersFile.open(path);
        if (answersFile.is_open()) {
            savedPath = path;
            break;
        }
    }

    if (!answersFile.is_open()) {
        // Если ни один путь не сработал, создаем в текущей директории
        answersFile.open("answers.json");
        savedPath = "answers.json";
    }

    answersFile << "{\n";
    answersFile << "  \"answers\": {\n";

    for (size_t i = 0; i < answers.size(); ++i) {
        std::string requestId = "request" +
                                std::string(3 - std::to_string(i + 1).length(), '0') +
                                std::to_string(i + 1);

        answersFile << "    \"" << requestId << "\": {\n";

        if (answers[i].empty()) {
            // Если result = false - только поле result
            answersFile << "      \"result\": \"false\"\n";
        } else {
            // Если result = true - поле result и массив relevance

            // СОРТИРУЕМ doc_id по возрастанию перед записью в JSON
            std::vector<std::pair<int, float>> sortedAnswers = answers[i];
            std::sort(sortedAnswers.begin(), sortedAnswers.end(),
                      [](const std::pair<int, float>& a, const std::pair<int, float>& b) {
                          return a.first < b.first; // сортировка по возрастанию doc_id
                      });

            answersFile << "      \"result\": \"true\",\n";
            answersFile << "      \"relevance\": [\n";

            for (size_t j = 0; j < sortedAnswers.size(); ++j) {
                answersFile << "        {\n";
                answersFile << "          \"docid\": " << sortedAnswers[j].first << ",\n";
                answersFile << "          \"rank\": " << sortedAnswers[j].second << "\n";
                answersFile << "        }";

                // Запятая между элементами массива, кроме последнего
                if (j < sortedAnswers.size() - 1) {
                    answersFile << ",";
                }
                answersFile << "\n";
            }

            answersFile << "      ]\n";
        }

        answersFile << "    }";

        // Запятая между запросами, кроме последнего
        if (i < answers.size() - 1) {
            answersFile << ",";
        }
        answersFile << "\n";
    }

    answersFile << "  }\n";
    answersFile << "}\n";
    answersFile.close();

    std::cout << "✓ Results saved to: " << savedPath << std::endl;
}