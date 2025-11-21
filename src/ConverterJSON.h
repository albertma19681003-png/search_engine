#pragma once

#include <string>
#include <vector>
#include <map>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include "nlohmann/json.hpp"

using json = nlohmann::json;

class ConverterJSON {
public:
    ConverterJSON();

    std::vector<std::string> GetTextDocuments();
    int GetResponsesLimit();
    std::vector<std::string> GetRequests();
    void putAnswers(std::vector<std::vector<std::pair<int, float>>> answers);

private:
    std::string configPath = "../resources/config.json";
    std::string requestsPath = "../resources/requests.json";
    std::string answersPath = "../resources/answers.json";

    void readConfig();
    std::string engineName;
    std::string version;
    int maxResponses;
    std::vector<std::string> files;
    
    // Константы для проверки версии
    const std::string EXPECTED_VERSION = "3.23";
};