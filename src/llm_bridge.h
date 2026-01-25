#pragma once
#ifndef CATA_SRC_LLM_BRIDGE_H
#define CATA_SRC_LLM_BRIDGE_H

// Disable OpenSSL support to avoid linking issues
#undef CPPHTTPLIB_OPENSSL_SUPPORT
#include "third-party/httplib/httplib.h"
#include "third-party/nlohmann/json.hpp"

#include <string>
#include <vector>
#include <optional>
#include <random>
#include <chrono>

namespace cata_llm
{

using json = nlohmann::json;

/**
 * Configuration for LLM and vector database connections
 */
struct LLMConfig {
    // Ollama settings
    std::string ollama_host = "172.29.64.1";
    int ollama_port = 11434;
    std::string llm_model = "PetrosStav/gemma3-tools:4b";
    std::string embed_model = "nomic-embed-text";

    // Qdrant settings
    std::string qdrant_host = "localhost";
    int qdrant_port = 6333;
    std::string collection_name = "cdda_npc_memory";
    int vector_size = 768;  // nomic-embed-text dimension

    // Timeouts (seconds)
    int connection_timeout = 5;
    int read_timeout = 60;  // LLM generation can be slow
};

/**
 * Result of a RAG query with optional action
 */
struct LLMResult {
    std::string response;
    std::string action;
    std::string target;
    bool success = false;
    std::string error;
};

/**
 * LLMBridge - Connects CDDA to Ollama LLM and Qdrant vector database
 *
 * Provides a 3-step RAG pipeline:
 * 1. get_embedding() - Convert text to vector via Ollama
 * 2. retrieve_memory() - Search Qdrant for similar memories
 * 3. query_llm() - Generate response with context via Ollama
 */
class LLMBridge
{
    public:
        explicit LLMBridge( const LLMConfig &config = LLMConfig{} );

        // Disable copy (httplib::Client is not copyable)
        LLMBridge( const LLMBridge & ) = delete;
        LLMBridge &operator=( const LLMBridge & ) = delete;

        // Move is OK
        LLMBridge( LLMBridge && ) = default;
        LLMBridge &operator=( LLMBridge && ) = default;

        // ===== Core RAG Functions =====

        /**
         * Get embedding vector for text using Ollama /api/embeddings
         * @param text Input text to embed
         * @return Vector of floats (768 dimensions for nomic-embed-text)
         */
        std::vector<float> get_embedding( const std::string &text );

        /**
         * Search Qdrant for memories similar to the embedding
         * @param embedding Vector to search for
         * @param limit Maximum number of results
         * @return Concatenated text of retrieved memories
         */
        std::string retrieve_memory( const std::vector<float> &embedding, int limit = 3 );

        /**
         * Query LLM with context and prompt
         * @param context Retrieved memories and NPC state
         * @param prompt User input
         * @param system_prompt Optional system prompt override
         * @return LLMResult with response, action, target
         */
        LLMResult query_llm( const std::string &context, const std::string &prompt,
                             const std::string &system_prompt = "" );

        /**
         * Query LLM and return raw JSON response
         * Use this for prompts expecting custom JSON formats (world events, quests, etc.)
         * @param prompt The full prompt to send
         * @param system_prompt System instructions for the LLM
         * @return Raw JSON string from the LLM (the inner response field from Ollama)
         */
        std::string query_llm_raw( const std::string &prompt, const std::string &system_prompt = "" );

        // ===== Qdrant Management =====

        /**
         * Ensure the Qdrant collection exists, create if not
         * @return true if collection exists or was created
         */
        bool ensure_collection_exists();

        /**
         * Store a memory in Qdrant with auto-generated embedding
         * @param text Memory text to store
         * @param metadata Optional JSON metadata string
         * @return true if stored successfully
         */
        bool store_memory( const std::string &text, const std::string &metadata = "" );

        // ===== Health Checks =====

        /**
         * Check if Ollama is reachable
         */
        bool check_ollama_connection();

        /**
         * Check if Qdrant is reachable
         */
        bool check_qdrant_connection();

        /**
         * Get the current configuration
         */
        const LLMConfig &get_config() const {
            return config_;
        }

    private:
        LLMConfig config_;
        httplib::Client ollama_client_;
        httplib::Client qdrant_client_;

        // Default system prompt for NPC dialogue
        static const std::string DEFAULT_SYSTEM_PROMPT;

        // Generate a unique ID for Qdrant points
        uint64_t generate_point_id();

        // Parse LLM JSON response into LLMResult
        LLMResult parse_llm_response( const std::string &response_body );
};

} // namespace cata_llm

#endif // CATA_SRC_LLM_BRIDGE_H
