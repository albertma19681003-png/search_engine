#include "ConverterJSON.h"
#include <sstream>
#include <algorithm>
#include <filesystem>

ConverterJSON::ConverterJSON() {
    readConfig();
}

void ConverterJSON::readConfig() {
    std::vector<std::string> possibleConfigPaths = {
        "../resources/config.json",
        "../../resources/config.json",
        "resources/config.json",
        "../config.json"
    };

    std::ifstream configFile;
    std::string foundPath;
    bool fileFound = false;

    // Проверка существования файла конфигурации
    for (const auto& path : possibleConfigPaths) {
        configFile.open(path);
        if (configFile.is_open()) {
            foundPath = path;
            fileFound = true;
            std::cout << "✓ Found config file at: " << path << std::endl;
            break;
        }
    }

    if (!fileFound) {
        throw std::runtime_error("config file is missing");
    }

    // Чтение содержимого файла
    std::string content((std::istreambuf_iterator<char>(configFile)),
                        std::istreambuf_iterator<char>());
    configFile.close();

    // Проверка на пустой файл
    if (content.empty()) {
        throw std::runtime_error("config file is empty");
    }

    // Парсинг JSON
    json configJson;
    try {
        configJson = json::parse(content);
    } catch (const json::parse_error& e) {
        throw std::runtime_error("config file has invalid JSON format");
    }

    // Проверка наличия секции "config"
    if (!configJson.contains("config") || configJson["config"].is_null()) {
        throw std::runtime_error("config file is empty");
    }

    json configSection = configJson["config"];

    // Проверка и чтение поля "name"
    if (!configSection.contains("name") || !configSection["name"].is_string()) {
        throw std::runtime_error("config file is empty: missing 'name' field");
    }
    engineName = configSection["name"].get<std::string>();

    // Проверка и чтение поля "version"
    if (!configSection.contains("version") || !configSection["version"].is_string()) {
        throw std::runtime_error("config file has incorrect file version: missing 'version' field");
    }
    version = configSection["version"].get<std::string>();
    
    // Проверка соответствия версии
    if (version != EXPECTED_VERSION) {
        throw std::runtime_error("config.json has incorrect file version. Expected: " + EXPECTED_VERSION);
    }

    // Чтение поля "max_responses" (необязательное поле)
    if (configSection.contains("max_responses") && configSection["max_responses"].is_number()) {
        maxResponses = configSection["max_responses"].get<int>();
        if (maxResponses <= 0) {
            std::cout << "⚠️  Warning: max_responses must be positive, using default: 5" << std::endl;
            maxResponses = 5;
        }
    } else {
        std::cout << "ℹ️  max_responses not found in config, using default: 5" << std::endl;
        maxResponses = 5;
    }

    // Проверка и чтение секции "files"
    if (!configJson.contains("files") || !configJson["files"].is_array()) {
        throw std::runtime_error("config file is empty: missing 'files' section");
    }

    files.clear();
    for (const auto& file : configJson["files"]) {
        if (file.is_string()) {
            std::string filePath = file.get<std::string>();
            
            // Корректировка пути к файлам
            std::string correctedPath;
            if (filePath.find("../") == 0) {
                correctedPath = filePath;
            } else if (filePath.find("resources/") == 0) {
                correctedPath = "../" + filePath;
            } else {
                correctedPath = "../resources/" + filePath;
            }
            
            files.push_back(correctedPath);
        }
    }

    if (files.empty()) {
        throw std::runtime_error("config file is empty: no files specified in 'files' section");
    }

    // Вывод информации о конфигурации
    std::cout << "Starting " << engineName << " version " << version << std::endl;
    std::cout << "Max responses: " << maxResponses << std::endl;
    std::cout << "Found " << files.size() << " files to index" << std::endl;
}

std::vector<std::string> ConverterJSON::GetTextDocuments() {
    std::vector<std::string> documents;

    for (size_t i = 0; i < files.size(); ++i) {
        std::vector<std::string> possibleFilePaths = {
            files[i],
            "../" + files[i],
            "../../" + files[i],
            "resources/" + files[i].substr(files[i].find_last_of("/") + 1)
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
            documents.push_back("");
        }
    }

    return documents;
}

int ConverterJSON::GetResponsesLimit() {
    return maxResponses;
}

std::vector<std::string> ConverterJSON::GetRequests() {
    std::vector<std::string> possibleRequestPaths = {
        "../resources/requests.json",
        "../../resources/requests.json", 
        "resources/requests.json",
        "../requests.json"
    };

    std::ifstream requestsFile;
    std::string foundPath;
    bool fileFound = false;

    for (const auto& path : possibleRequestPaths) {
        requestsFile.open(path);
        if (requestsFile.is_open()) {
            foundPath = path;
            fileFound = true;
            std::cout << "✓ Found requests file at: " << path << std::endl;
            break;
        }
    }

    if (!fileFound) {
        throw std::runtime_error("requests.json file is missing");
    }

    // Парсинг JSON файла запросов
    json requestsJson;
    try {
        requestsFile >> requestsJson;
        requestsFile.close();
    } catch (const json::parse_error& e) {
        throw std::runtime_error("requests.json has invalid JSON format");
    }

    // Проверка наличия секции "requests"
    if (!requestsJson.contains("requests") || !requestsJson["requests"].is_array()) {
        throw std::runtime_error("requests.json is empty: missing 'requests' section");
    }

    std::vector<std::string> requests;
    for (const auto& request : requestsJson["requests"]) {
        if (request.is_string()) {
            requests.push_back(request.get<std::string>());
        }
    }

    return requests;
}
void ConverterJSON::putAnswers(std::vector<std::vector<std::pair<int, float>>> answers) {
    std::vector<std::string> possibleAnswerPaths = {
        "../resources/answers.json",
        "../../resources/answers.json",
        "resources/answers.json"
    };

    std::ofstream answersFile;
    std::string savedPath;

    // Создаем или перезаписываем файл answers.json
    for (const auto& path : possibleAnswerPaths) {
        answersFile.open(path);
        if (answersFile.is_open()) {
            savedPath = path;
            break;
        }
    }

    if (!answersFile.is_open()) {
        answersFile.open("answers.json");
        savedPath = "answers.json";
    }

    // Создаем JSON вручную для точного контроля формата
    answersFile << "{\n";
    answersFile << "  \"answers\": {\n";

    for (size_t i = 0; i < answers.size(); ++i) {
        std::string requestId = "request" + 
                               std::string(3 - std::to_string(i + 1).length(), '0') + 
                               std::to_string(i + 1);

        answersFile << "    \"" << requestId << "\": {\n";
        answersFile << "      \"result\": \"" << (answers[i].empty() ? "false" : "true") << "\"";

        if (!answers[i].empty()) {
            if (answers[i].size() == 1) {
                // Для одного документа
                answersFile << ",\n      \"docid\": " << answers[i][0].first;
                answersFile << ",\n      \"rank\": " << answers[i][0].second;
            } else {
                // Для нескольких документов - массив relevance
                answersFile << ",\n      \"relevance\": [\n";
                
                for (size_t j = 0; j < answers[i].size(); ++j) {
                    answersFile << "        { \"docid\": " << answers[i][j].first 
                               << ", \"rank\": " << answers[i][j].second << " }";
                    
                    if (j < answers[i].size() - 1) {
                        answersFile << ",";
                    }
                    answersFile << "\n";
                }
                
                answersFile << "      ]";
            }
        }

        answersFile << "\n    }";

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

