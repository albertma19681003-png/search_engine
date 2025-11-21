#include "gtest/gtest.h"
#include "../src/InvertedIndex.h"
#include "../src/SearchServer.h"
#include <vector>
#include <memory>

using namespace std;

void TestInvertedIndexFunctionality(
        const vector<string>& docs,
        const vector<string>& requests,
        const std::vector<vector<Entry>>& expected) {

    std::vector<std::vector<Entry>> result;
    InvertedIndex idx;
    idx.UpdateDocumentBase(docs);

    for(auto& request : requests) {
        std::vector<Entry> word_count = idx.GetWordCount(request);
        result.push_back(word_count);
    }

    ASSERT_EQ(result, expected);
}

TEST(TestCaseInvertedIndex, TestBasic) {
    const vector<string> docs = {
            "london is the capital of great britain",
            "big ben is the nickname for the Great bell of the striking clock"
    };

    const vector<string> requests = {"london", "the"};
    const vector<vector<Entry>> expected = {
            { {0, 1} },
            { {0, 1}, {1, 3} }
    };

    TestInvertedIndexFunctionality(docs, requests, expected);
}

TEST(TestCaseInvertedIndex, TestBasic2) {
    const vector<string> docs = {
            "milk milk milk milk water water water",
            "milk water water",
            "milk milk milk milk milk water water water water water",
            "americano cappuccino"
    };

    const vector<string> requests = {"milk", "water", "cappuccino"};
    const vector<vector<Entry>> expected = {
            { {0, 4}, {1, 1}, {2, 5} },
            { {0, 3}, {1, 2}, {2, 5} },
            { {3, 1} }
    };

    TestInvertedIndexFunctionality(docs, requests, expected);
}

TEST(TestCaseInvertedIndex, TestInvertedIndexMissingWord) {
    const vector<string> docs = {
            "a b c d e f g h i j k l",
            "statement"
    };

    const vector<string> requests = {"m", "statement"};
    const vector<vector<Entry>> expected = {
            { },
            { {1, 1} }
    };

    TestInvertedIndexFunctionality(docs, requests, expected);
}

TEST(TestCaseSearchServer, TestSimple) {
    const vector<string> docs = {
            "milk milk milk milk water water water",
            "milk water water",
            "milk milk milk milk milk water water water water water",
            "americano cappuccino"
    };

    const vector<string> request = {"milk water", "sugar"};
    
    const std::vector<vector<RelativeIndex>> expected = {
        {
            {2, 1.0f},
            {0, 0.7f},
            {1, 0.3f}
        },
        { }
    };

    auto idx = std::make_shared<InvertedIndex>();
    idx->UpdateDocumentBase(docs);
    SearchServer srv(idx);
    std::vector<vector<RelativeIndex>> result = srv.search(request);

    ASSERT_EQ(result.size(), expected.size());
    for (size_t i = 0; i < result.size(); ++i) {
        ASSERT_EQ(result[i].size(), expected[i].size());
        for (size_t j = 0; j < result[i].size(); ++j) {
            EXPECT_EQ(result[i][j].doc_id, expected[i][j].doc_id);
            EXPECT_NEAR(result[i][j].rank, expected[i][j].rank, 0.001f);
        }
    }
}

TEST(TestCaseSearchServer, TestTop5) {
    const vector<string> docs = {
            "london is the capital of great britain",
            "paris is the capital of france",
            "berlin is the capital of germany",
            "rome is the capital of italy",
            "madrid is the capital of spain",
            "lisboa is the capital of portugal",
            "bern is the capital of switzerland",
            "moscow is the capital of russia",
            "kiev is the capital of ukraine",
            "minsk is the capital of belarus",
            "astana is the capital of kazakhstan",
            "beijing is the capital of china",
            "tokyo is the capital of japan",
            "bangkok is the capital of thailand",
            "welcome to moscow the capital of russia the third rome",
            "amsterdam is the capital of netherlands",
            "helsinki is the capital of finland",
            "oslo is the capital of norway",
            "stockholm is the capital of sweden",
            "riga is the capital of latvia",
            "tallinn is the capital of estonia",
            "warsaw is the capital of poland",
    };

    const vector<string> request = {"moscow is the capital of russia"};
    
    // Исправленные ожидания - только документ 7 содержит ВСЕ слова
    const std::vector<vector<RelativeIndex>> expected = {
        {
            {7, 1.0f}
        }
    };

    auto idx = std::make_shared<InvertedIndex>();
    idx->UpdateDocumentBase(docs);
    SearchServer srv(idx);
    std::vector<vector<RelativeIndex>> result = srv.search(request);

    ASSERT_EQ(result.size(), expected.size());
    if (!result.empty()) {
        ASSERT_EQ(result[0].size(), expected[0].size());
        for (size_t j = 0; j < result[0].size(); ++j) {
            EXPECT_EQ(result[0][j].doc_id, expected[0][j].doc_id);
            EXPECT_NEAR(result[0][j].rank, expected[0][j].rank, 0.001f);
        }
    }
}

TEST(TestCaseSearchServer, TestBasicSearch) {
    const vector<string> docs = {
            "apple banana orange",
            "banana cherry", 
            "orange apple apple",
            "grape fruit"
    };

    const vector<string> request = {"apple banana"};

    const std::vector<vector<RelativeIndex>> expected = {
        {
            {0, 1.0f}
        }
    };

    auto idx = std::make_shared<InvertedIndex>();
    idx->UpdateDocumentBase(docs);
    SearchServer srv(idx);
    std::vector<vector<RelativeIndex>> result = srv.search(request);

    ASSERT_EQ(result.size(), expected.size());
    if (!result.empty()) {
        ASSERT_EQ(result[0].size(), expected[0].size());
        for (size_t j = 0; j < result[0].size(); ++j) {
            EXPECT_EQ(result[0][j].doc_id, expected[0][j].doc_id);
            EXPECT_NEAR(result[0][j].rank, expected[0][j].rank, 0.01f);
        }
    }
}

TEST(TestCaseSearchServer, TestExactCalculation) {
    const vector<string> docs = {
            "word1 word1 word2",
            "word1 word2 word2",
            "word1 word1 word1",
            "word2 word2 word2"
    };

    const vector<string> request = {"word1 word2"};

    const std::vector<vector<RelativeIndex>> expected = {
        {
            {0, 1.0f},
            {1, 1.0f}
        }
    };

    auto idx = std::make_shared<InvertedIndex>();
    idx->UpdateDocumentBase(docs);
    SearchServer srv(idx);
    std::vector<vector<RelativeIndex>> result = srv.search(request);

    ASSERT_EQ(result.size(), expected.size());
    if (!result.empty()) {
        ASSERT_EQ(result[0].size(), expected[0].size());
        for (size_t j = 0; j < result[0].size(); ++j) {
            EXPECT_NEAR(result[0][j].rank, 1.0f, 0.001f);
        }
    }
}

TEST(TestCaseSearchServer, TestEmptyQuery) {
    const vector<string> docs = {
            "some text here",
            "another text there"
    };

    const vector<string> request = {""};
    const std::vector<vector<RelativeIndex>> expected = {
            { }
    };

    auto idx = std::make_shared<InvertedIndex>();
    idx->UpdateDocumentBase(docs);
    SearchServer srv(idx);
    std::vector<vector<RelativeIndex>> result = srv.search(request);

    ASSERT_EQ(result, expected);
}

TEST(sample_test_case, sample_test) {
    EXPECT_EQ(1, 1);
}
