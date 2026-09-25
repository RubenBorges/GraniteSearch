#pragma once
#ifndef GRANITE_EMBEDDER_HPP
#define GRANITE_EMBEDDER_HPP

#include <vector>
#include <string>
#include "llama.h"

class GraniteEmbedder {
private:
    llama_model* model = nullptr;
    llama_context* ctx = nullptr;
    const int EMBEDDING_DIM = 384;

public:
    // Default parameter sits safely inside header declaration layer
    GraniteEmbedder(const std::string& model_path, int max_ctx = 512);
    ~GraniteEmbedder();

    std::vector<float> compute_embedding(const std::string& text);
    int get_dimension() const;
};

#endif
