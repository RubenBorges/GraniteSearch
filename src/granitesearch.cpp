#include <granitesearch.hpp>

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <graniteembedder.hpp>

namespace {

struct RawChunk {
	std::string text;
	std::string source_doc;
};

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

std::vector<std::string> split_into_sentences(const std::string& text) {
	std::vector<std::string> sentences;
	const std::regex sentence_regex("[^.!?]+([.!?]+|$)\\s*");

	for (auto iterator = std::sregex_iterator(text.begin(), text.end(), sentence_regex);
		 iterator != std::sregex_iterator(); ++iterator) {
		std::string sentence = iterator->str();
		const size_t first_non_space = sentence.find_first_not_of(" \\n\\r\\t");
		if (first_non_space == std::string::npos) {
			continue;
		}
		const size_t last_non_space = sentence.find_last_not_of(" \\n\\r\\t");
		sentence = sentence.substr(first_non_space, last_non_space - first_non_space + 1);
		sentences.push_back(std::move(sentence));
	}
	return sentences;
}

double calculate_cosine_similarity(const std::vector<float>& first,
                                   const std::vector<float>& second) {
	if (first.size() != second.size()) {
		throw std::invalid_argument("Vectors must be of the same dimension.");
	}

	double dot_product = 0.0;
	double first_magnitude = 0.0;
	double second_magnitude = 0.0;
	for (size_t index = 0; index < first.size(); ++index) {
		dot_product += static_cast<double>(first[index] * second[index]);
		first_magnitude += static_cast<double>(first[index] * first[index]);
		second_magnitude += static_cast<double>(second[index] * second[index]);
	}

	first_magnitude = std::sqrt(first_magnitude);
	second_magnitude = std::sqrt(second_magnitude);
	if (first_magnitude == 0.0 || second_magnitude == 0.0) {
		return 0.0;
	}
	return dot_product / (first_magnitude * second_magnitude);
}

} // namespace

std::string run_semantic_search(const std::string& model_path,
                                const std::string& query,
                                const std::vector<std::string>& document_paths) {
	std::ostringstream output;
	std::vector<RawChunk> database_chunks;

	for (const std::string& document_path : document_paths) {
		std::ifstream file_stream(document_path);
		if (!file_stream.is_open()) {
			output << "[Warning]: Could not open file \"" << document_path
				   << "\". Skipping.\n";
			continue;
		}

		std::stringstream buffer;
		buffer << file_stream.rdbuf();
		for (const std::string& sentence : split_into_sentences(buffer.str())) {
			database_chunks.push_back({sentence, document_path});
		}
	}

	try {
		GraniteEmbedder embedder(model_path);
		output << "[System]: Encoding " << database_chunks.size()
			   << " database chunks...\n";

		std::vector<EmbeddedChunk> vector_database;
		vector_database.reserve(database_chunks.size());
		for (const RawChunk& chunk : database_chunks) {
			try {
				vector_database.push_back({chunk.text, chunk.source_doc,
					embedder.compute_embedding(chunk.text)});
			} catch (const std::exception& exception) {
				output << "Error embedding chunk \"" << chunk.text << "\": "
				       << exception.what() << '\n';
			}
		}

		output << "[System]: Computing embedding for input query: \""
			   << query << "\"\n";
		const std::vector<float> query_vector = embedder.compute_embedding(query);

		std::vector<SearchResult> results;
		results.reserve(vector_database.size());
		for (const EmbeddedChunk& item : vector_database) {
			results.push_back({calculate_cosine_similarity(query_vector, item.embedding) * 100.0,
				item.text, item.source_doc});
		}
		std::sort(results.begin(), results.end(), [](const SearchResult& first,
		                                             const SearchResult& second) {
			return first.score > second.score;
		});

		output << "\n=============================================\n"
		       << "        RANKED SEMANTIC SEARCH RESULTS       \n"
		       << "=============================================\n"
		       << "Query: \"" << query << "\"\n\n";
		if (results.empty()) {
			output << " [System Notice]: No chunks were successfully loaded or matched.\n";
		} else {
			output << std::fixed << std::setprecision(2);
			const size_t display_limit = std::min<size_t>(5, results.size());
			for (size_t index = 0; index < display_limit; ++index) {
				output << "[#" << index + 1 << "] Match Score: " << results[index].score
				       << "% | Origin File: " << results[index].source_doc << "\n"
				       << "     Chunk: \"" << results[index].text << "\"\n\n";
			}
		}
		output << "=============================================\n";
	} catch (const std::exception& exception) {
		output << "Fatal Exception: " << exception.what() << '\n';
	}

	return output.str();
}
