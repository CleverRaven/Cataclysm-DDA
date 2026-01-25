/**
 * Unit tests for LLM Bridge using Catch2
 *
 * These tests require Ollama and Qdrant to be running.
 * Tagged with [llm] and [nogame] for selective running.
 *
 * Run with: ./cata_test "[llm]"
 */

#include "cata_catch.h"
#include "llm_bridge.h"

#include <chrono>
#include <thread>

using namespace cata_llm;

// Skip tests if services are not available
static bool services_available()
{
    static bool checked = false;
    static bool available = false;

    if( !checked ) {
        LLMBridge bridge;
        available = bridge.check_ollama_connection() && bridge.check_qdrant_connection();
        checked = true;
        if( !available ) {
            WARN( "LLM services not available - skipping LLM tests" );
        }
    }
    return available;
}

TEST_CASE( "LLMBridge_default_config", "[llm][nogame]" )
{
    LLMConfig config;

    CHECK( config.ollama_host == "172.29.64.1" );
    CHECK( config.ollama_port == 11434 );
    CHECK( config.qdrant_host == "172.29.64.1" );
    CHECK( config.qdrant_port == 6333 );
    CHECK( config.vector_size == 768 );
    CHECK( config.embed_model == "nomic-embed-text" );
}

TEST_CASE( "LLMBridge_construction", "[llm][nogame]" )
{
    // Default construction should not throw
    REQUIRE_NOTHROW( LLMBridge() );

    // Custom config construction
    LLMConfig custom;
    custom.ollama_host = "localhost";
    custom.ollama_port = 12345;
    REQUIRE_NOTHROW( LLMBridge( custom ) );
}

TEST_CASE( "LLMBridge_ollama_connection", "[llm][nogame][network]" )
{
    if( !services_available() ) {
        WARN( "Ollama not available - skipping test" ); return;
    }

    LLMBridge bridge;
    REQUIRE( bridge.check_ollama_connection() );
}

TEST_CASE( "LLMBridge_qdrant_connection", "[llm][nogame][network]" )
{
    if( !services_available() ) {
        WARN( "Qdrant not available - skipping test" ); return;
    }

    LLMBridge bridge;
    REQUIRE( bridge.check_qdrant_connection() );
}

TEST_CASE( "LLMBridge_embedding", "[llm][nogame][network]" )
{
    if( !services_available() ) {
        WARN( "Services not available - skipping test" ); return;
    }

    LLMBridge bridge;

    SECTION( "basic embedding" ) {
        auto embedding = bridge.get_embedding( "Hello, world!" );
        REQUIRE( embedding.size() == 768 );

        // Embeddings should be normalized (approximately)
        float sum_sq = 0.0f;
        for( float v : embedding ) {
            sum_sq += v * v;
        }
        // Magnitude should be close to 1 for normalized vectors
        CHECK( sum_sq > 0.5f );
        CHECK( sum_sq < 1.5f );
    }

    SECTION( "empty text embedding" ) {
        auto embedding = bridge.get_embedding( "" );
        // Should still return an embedding
        CHECK( embedding.size() == 768 );
    }

    SECTION( "long text embedding" ) {
        std::string long_text( 1000, 'a' );
        auto embedding = bridge.get_embedding( long_text );
        REQUIRE( embedding.size() == 768 );
    }
}

TEST_CASE( "LLMBridge_collection_management", "[llm][nogame][network]" )
{
    if( !services_available() ) {
        WARN( "Services not available - skipping test" ); return;
    }

    LLMBridge bridge;

    SECTION( "ensure collection exists" ) {
        // Should create collection if not exists, or succeed if exists
        REQUIRE( bridge.ensure_collection_exists() );
        // Calling again should succeed (idempotent)
        REQUIRE( bridge.ensure_collection_exists() );
    }
}

TEST_CASE( "LLMBridge_store_and_retrieve", "[llm][nogame][network]" )
{
    if( !services_available() ) {
        WARN( "Services not available - skipping test" ); return;
    }

    LLMBridge bridge;

    // Ensure collection exists first
    REQUIRE( bridge.ensure_collection_exists() );

    SECTION( "store memory" ) {
        bool stored = bridge.store_memory(
                          "The survivor traded medical supplies for ammunition.",
                          R"({"npc": "TestNPC", "type": "trade"})"
                      );
        REQUIRE( stored );
    }

    SECTION( "store and retrieve" ) {
        // Store a unique memory
        std::string unique_memory = "Test memory created at timestamp " +
                                    std::to_string( std::time( nullptr ) );
        REQUIRE( bridge.store_memory( unique_memory ) );

        // Small delay to ensure indexing
        std::this_thread::sleep_for( std::chrono::milliseconds( 100 ) );

        // Retrieve using similar query
        auto embedding = bridge.get_embedding( unique_memory );
        REQUIRE( !embedding.empty() );

        std::string retrieved = bridge.retrieve_memory( embedding, 1 );
        // Should find the memory we just stored
        CHECK( !retrieved.empty() );
    }
}

TEST_CASE( "LLMBridge_llm_query", "[llm][nogame][network][slow]" )
{
    if( !services_available() ) {
        WARN( "Services not available - skipping test" ); return;
    }

    LLMBridge bridge;

    SECTION( "basic query" ) {
        auto result = bridge.query_llm( "", "Hello, who are you?" );
        CHECK( result.success );
        CHECK( !result.response.empty() );
    }

    SECTION( "query with context" ) {
        std::string context = "- The player helped you yesterday\n- You are grateful\n";
        auto result = bridge.query_llm( context, "How do you feel about me?" );
        CHECK( result.success );
        CHECK( !result.response.empty() );
    }

    SECTION( "action parsing" ) {
        std::string context = "- The player just insulted you gravely\n";
        auto result = bridge.query_llm( context, "You're a worthless coward!" );
        CHECK( result.success );
        // Action could be MAKE_ANGRY or similar, but depends on LLM
        // Just check we got a valid response
        CHECK( !result.response.empty() );
    }
}

TEST_CASE( "LLMBridge_full_rag_pipeline", "[llm][nogame][network][slow]" )
{
    if( !services_available() ) {
        WARN( "Services not available - skipping test" ); return;
    }

    LLMBridge bridge;
    REQUIRE( bridge.ensure_collection_exists() );

    // Store some memories
    bridge.store_memory( "The player saved my life from zombies last week." );
    bridge.store_memory( "I gave the player directions to the nearby shelter." );

    // Small delay for indexing
    std::this_thread::sleep_for( std::chrono::milliseconds( 200 ) );

    // Full RAG pipeline
    std::string user_input = "Do you remember when we first met?";

    // 1. Embed
    auto embedding = bridge.get_embedding( user_input );
    REQUIRE( embedding.size() == 768 );

    // 2. Retrieve
    std::string memories = bridge.retrieve_memory( embedding, 3 );
    // Memories might be empty if vector search didn't find anything above threshold
    INFO( "Retrieved memories: " << memories );

    // 3. Query LLM
    auto result = bridge.query_llm( memories, user_input );
    CHECK( result.success );
    CHECK( !result.response.empty() );

    INFO( "LLM Response: " << result.response );
    INFO( "Action: " << result.action );
}
