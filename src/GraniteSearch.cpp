#include <iostream>
#include <vector>
#include <cmath>
#include <stdexcept>
#include <string>
#include <iomanip>
#include <format>
#include <algorithm>
#include <print>
#include <regex>
#include <fstream>
#include <sstream>
#include <graniteembedder.hpp>

// Simple, effective rule-based sentence chunker for C++
std::vector<std::string> split_into_sentences(const std::string& text) {
    std::vector<std::string> sentences;
    std::regex sentence_regex("[^.!?]+([.!?]+|$)\\s*");
    
    auto words_begin = std::sregex_iterator(text.begin(), text.end(), sentence_regex);
    auto words_end = std::sregex_iterator();

    for (std::sregex_iterator i = words_begin; i != words_end; ++i) {
        std::smatch match = *i;
        std::string str = match.str();
        
        str.erase(str.find_last_not_of(" \n\r\t") + 1);
        if (!str.empty()) {
            sentences.push_back(str);
        }
    }
    return sentences;
}

// Calculates the Cosine Similarity between two 384-dimensional arrays
double calculate_cosine_similarity(const std::vector<float>& vecA, const std::vector<float>& vecB) {
    if (vecA.size() != vecB.size()) {
        throw std::invalid_argument("Vectors must be of the same dimension.");
    }

    double dot_product = 0.0;
    double magnitude_A = 0.0;
    double magnitude_B = 0.0;

    for (size_t i = 0; i < vecA.size(); ++i) {
        dot_product += static_cast<double>(vecA[i] * vecB[i]);
        magnitude_A += static_cast<double>(vecA[i] * vecA[i]);
        magnitude_B += static_cast<double>(vecB[i] * vecB[i]);
    }

    magnitude_A = std::sqrt(magnitude_A);
    magnitude_B = std::sqrt(magnitude_B);

    if (magnitude_A == 0.0 || magnitude_B == 0.0) {
        return 0.0;
    }

    return dot_product / (magnitude_A * magnitude_B);
}

// Struct to store un-embedded text metadata pairs
struct RawChunk {
    std::string text;
    std::string source_doc; 
};

// Struct to guarantee sentences and vectors never lose alignment
struct EmbeddedChunk {
    std::string text;
    std::string source_doc; 
    std::vector<float> embedding;
};

struct SearchResult {
    double score;
    std::string text;
    std::string source_doc; 
};

int main(int argc, char* argv[]) {
    // Establish basic command-line instructions
    if (argc < 3) {
        std::println("Usage:   {} [path_to_gguf_model] \"[query]\" [file1.txt] [file2.txt] ...", argv[0]);
        std::println("Example: {} ./dist/ibm-granite30m/ibm-granite \"Linda dog\" dist/docs/linda_the_dog.txt\n", argv[0]);
        return 1;
    }

    // STEP 0: PARSE COMMAND LINE
    const std::string model_path = argv[1];
    std::string inputQuery{argv[2]};

    std::vector<RawChunk> databaseChunks; 
    
    // Modernized Loop: Open and read target files natively from disk
    for (int i = 3; i < argc; ++i) {
        std::string filePath = argv[i];
        std::ifstream fileStream(filePath);
        
        if (!fileStream.is_open()) {
            std::println("[Warning]: Could not open file \"{}\". Skipping.", filePath);
            continue;
        }

        // Slurp the whole file text structure into a string stream
        std::stringstream buffer;
        buffer << fileStream.rdbuf();
        std::string fileContent = buffer.str();
        fileStream.close();
        
        // Chunk the file text and track the exact filename as its origin ID!
        std::vector<std::string> chunks = split_into_sentences(fileContent);
        for (const auto& sentence : chunks) {
            databaseChunks.push_back({sentence, filePath});
        }
    }

    std::vector<EmbeddedChunk> vectorDatabase;
    
    try {
        // STEP 1: Load up the model using the updated library
        GraniteEmbedder embedder(model_path);
        
        // Loop over and embed every isolated sentence chunk dynamically
        std::println("[System]: Encoding {} database chunks...", databaseChunks.size());
        for (const auto& chunk : databaseChunks) {
            try {
                std::vector<float> emb = embedder.compute_embedding(chunk.text);
                vectorDatabase.push_back({chunk.text, chunk.source_doc, emb}); 
            } catch (const std::exception& e) {
                std::println("Error embedding chunk \"{}\": {}", chunk.text, e.what());
            }
        }

        // Embed user query
        std::println("[System]: Computing embedding for input query: \"{}\"", inputQuery);
        std::vector<float> queryVector = embedder.compute_embedding(inputQuery);
        
        // COMPARE QUERY against database chunks safely
        std::vector<SearchResult> results;
        results.reserve(vectorDatabase.size()); 

        for (const auto& item : vectorDatabase) {
            double similarity = calculate_cosine_similarity(queryVector, item.embedding);
            results.push_back({similarity * 100.0, item.text, item.source_doc});
        }

        // C++26/C++23 Lambda sorting
        std::sort(results.begin(), results.end(), [](const SearchResult& a, const SearchResult& b) {
            return a.score > b.score; 
        });

        // 4. Output the top ranked matches cleanly
        std::println("\n=============================================");
        std::println("        RANKED SEMANTIC SEARCH RESULTS       ");
        std::println("=============================================");
        std::println("Query: \"{}\"\n", inputQuery);

        if (results.empty()) {
            std::println(" [System Notice]: No chunks were successfully loaded or matched.");
        } else {
            size_t display_limit = std::min<size_t>(5, results.size());
            for (size_t i = 0; i < display_limit; ++i) {
                std::println("[#{}] Match Score: {:.2f}% | Origin File: {}", i + 1, results[i].score, results[i].source_doc);
                std::println("     Chunk: \"{}\"\n", results[i].text);
            }
        }
        std::println("=============================================\n");

    } catch (const std::exception& e) {
        std::println("Fatal Exception: {}", e.what());
        return 1;
    }
    
    return 0;
}