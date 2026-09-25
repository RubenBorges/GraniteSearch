#pragma once

#include <string>
#include <vector>

std::string run_semantic_search(const std::string& model_path,
                                const std::string& query,
                                const std::vector<std::string>& document_paths);
