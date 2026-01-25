#include "llm_bridge.h"

#include <sstream>
#include <iostream>

namespace cata_llm
{

// Default system prompt for NPC dialogue with JSON output
const std::string LLMBridge::DEFAULT_SYSTEM_PROMPT = R"(You are an NPC in Cataclysm: Dark Days Ahead, a post-apocalyptic survival roguelike game.
Stay in character based on your personality and the context provided.

You MUST respond with valid JSON in this exact format:
{
  "response": "Your dialogue text here",
  "action": "NONE",
  "target": "player"
}

Valid actions:
- NONE: Just talk, no special action
- MAKE_ANGRY: Become hostile to the target
- RUN_AWAY: Flee from the target
- GIVE_QUEST: Offer a quest to the target
- TRADE: Initiate trading with the target
- FOLLOW: Start following the target

Keep responses concise and appropriate for the game's tone.)";

LLMBridge::LLMBridge( const LLMConfig &config )
    : config_( config )
    , ollama_client_( config.ollama_host, config.ollama_port )
    , qdrant_client_( config.qdrant_host, config.qdrant_port )
{
    // Configure timeouts
    ollama_client_.set_connection_timeout( config_.connection_timeout );
    ollama_client_.set_read_timeout( config_.read_timeout );

    qdrant_client_.set_connection_timeout( config_.connection_timeout );
    qdrant_client_.set_read_timeout( config_.read_timeout );
}

// ===== Health Checks =====

bool LLMBridge::check_ollama_connection()
{
    auto res = ollama_client_.Get( "/api/tags" );
    return res && res->status == 200;
}

bool LLMBridge::check_qdrant_connection()
{
    auto res = qdrant_client_.Get( "/collections" );
    return res && res->status == 200;
}

// ===== Core RAG Functions =====

std::vector<float> LLMBridge::get_embedding( const std::string &text )
{
    std::vector<float> result;

    json request = {
        {"model", config_.embed_model},
        {"prompt", text}
    };

    auto res = ollama_client_.Post( "/api/embeddings", request.dump(), "application/json" );

    if( !res ) {
        std::cerr << "[LLMBridge] Embedding request failed: connection error" << std::endl;
        return result;
    }

    if( res->status != 200 ) {
        std::cerr << "[LLMBridge] Embedding request failed: HTTP " << res->status << std::endl;
        return result;
    }

    try {
        json response = json::parse( res->body );
        if( response.contains( "embedding" ) && response["embedding"].is_array() ) {
            result = response["embedding"].get<std::vector<float>>();
        }
    } catch( const json::exception &e ) {
        std::cerr << "[LLMBridge] JSON parse error: " << e.what() << std::endl;
    }

    return result;
}

std::string LLMBridge::retrieve_memory( const std::vector<float> &embedding, int limit )
{
    if( embedding.empty() ) {
        return "";
    }

    std::string endpoint = "/collections/" + config_.collection_name + "/points/search";

    json request = {
        {"vector", embedding},
        {"limit", limit},
        {"with_payload", true}
    };

    auto res = qdrant_client_.Post( endpoint, request.dump(), "application/json" );

    if( !res || res->status != 200 ) {
        // Collection might not exist yet, not an error
        return "";
    }

    std::stringstream memories;

    try {
        json response = json::parse( res->body );
        if( response.contains( "result" ) && response["result"].is_array() ) {
            for( const auto &point : response["result"] ) {
                if( point.contains( "payload" ) && point["payload"].contains( "text" ) ) {
                    memories << "- " << point["payload"]["text"].get<std::string>() << "\n";
                }
            }
        }
    } catch( const json::exception &e ) {
        std::cerr << "[LLMBridge] Memory retrieval parse error: " << e.what() << std::endl;
    }

    return memories.str();
}

LLMResult LLMBridge::query_llm( const std::string &context, const std::string &prompt,
                                const std::string &system_prompt )
{
    LLMResult result;

    std::string effective_system = system_prompt.empty() ? DEFAULT_SYSTEM_PROMPT : system_prompt;

    // Build the full prompt with context
    std::stringstream full_prompt;
    if( !context.empty() ) {
        full_prompt << "CONTEXT (relevant memories):\n" << context << "\n\n";
    }
    full_prompt << "USER: " << prompt;

    json request = {
        {"model", config_.llm_model},
        {"prompt", full_prompt.str()},
        {"system", effective_system},
        {"stream", false},  // Disable streaming for simpler parsing
        {"format", "json"}  // Request JSON output
    };

    auto res = ollama_client_.Post( "/api/generate", request.dump(), "application/json" );

    if( !res ) {
        result.error = "Connection error to Ollama";
        return result;
    }

    if( res->status != 200 ) {
        result.error = "HTTP " + std::to_string( res->status );
        return result;
    }

    return parse_llm_response( res->body );
}

std::string LLMBridge::query_llm_raw( const std::string &prompt, const std::string &system_prompt )
{
    std::string effective_system = system_prompt.empty() ?
                                   "You are an AI in a post-apocalyptic world. Output valid JSON only." :
                                   system_prompt;

    json request = {
        {"model", config_.llm_model},
        {"prompt", prompt},
        {"system", effective_system},
        {"stream", false},
        {"format", "json"}
    };

    auto res = ollama_client_.Post( "/api/generate", request.dump(), "application/json" );

    if( !res ) {
        std::cerr << "[LLMBridge] Raw query failed: connection error" << std::endl;
        return "";
    }

    if( res->status != 200 ) {
        std::cerr << "[LLMBridge] Raw query failed: HTTP " << res->status << std::endl;
        return "";
    }

    try {
        json outer = json::parse( res->body );
        if( outer.contains( "response" ) ) {
            return outer["response"].get<std::string>();
        }
    } catch( const json::exception &e ) {
        std::cerr << "[LLMBridge] Raw query parse error: " << e.what() << std::endl;
    }

    return "";
}

LLMResult LLMBridge::parse_llm_response( const std::string &response_body )
{
    LLMResult result;

    try {
        json outer = json::parse( response_body );

        // Ollama wraps the response in a "response" field
        if( !outer.contains( "response" ) ) {
            result.error = "Missing 'response' field in Ollama output";
            return result;
        }

        std::string inner_json = outer["response"].get<std::string>();

        // Parse the inner JSON (the actual NPC response)
        json inner = json::parse( inner_json );

        if( inner.contains( "response" ) ) {
            result.response = inner["response"].get<std::string>();
        }
        if( inner.contains( "action" ) ) {
            result.action = inner["action"].get<std::string>();
        }
        if( inner.contains( "target" ) ) {
            result.target = inner["target"].get<std::string>();
        }

        result.success = !result.response.empty();

    } catch( const json::exception &e ) {
        // If JSON parsing fails, try to extract raw text
        try {
            json outer = json::parse( response_body );
            if( outer.contains( "response" ) ) {
                result.response = outer["response"].get<std::string>();
                result.action = "NONE";
                result.target = "player";
                result.success = true;
            }
        } catch( ... ) {
            result.error = "JSON parse error: " + std::string( e.what() );
        }
    }

    return result;
}

// ===== Qdrant Management =====

bool LLMBridge::ensure_collection_exists()
{
    std::string endpoint = "/collections/" + config_.collection_name;

    // Check if collection exists
    auto res = qdrant_client_.Get( endpoint );
    if( res && res->status == 200 ) {
        return true;  // Already exists
    }

    // Create collection
    json request = {
        {"vectors", {
                {"size", config_.vector_size},
                {"distance", "Cosine"}
            }
        }
    };

    res = qdrant_client_.Put( endpoint, request.dump(), "application/json" );

    if( !res ) {
        std::cerr << "[LLMBridge] Failed to create collection: connection error" << std::endl;
        return false;
    }

    if( res->status != 200 ) {
        std::cerr << "[LLMBridge] Failed to create collection: HTTP " << res->status << std::endl;
        return false;
    }

    return true;
}

uint64_t LLMBridge::generate_point_id()
{
    // Use timestamp + random for unique IDs
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::microseconds>(
                         now.time_since_epoch() ).count();

    std::random_device rd;
    std::mt19937_64 gen( rd() );
    std::uniform_int_distribution<uint64_t> dis( 0, 0xFFFF );

    return static_cast<uint64_t>( timestamp ) ^ ( dis( gen ) << 48 );
}

bool LLMBridge::store_memory( const std::string &text, const std::string &metadata )
{
    // First ensure collection exists
    if( !ensure_collection_exists() ) {
        return false;
    }

    // Get embedding for the text
    std::vector<float> embedding = get_embedding( text );
    if( embedding.empty() ) {
        std::cerr << "[LLMBridge] Failed to get embedding for memory" << std::endl;
        return false;
    }

    // Build the point
    json payload = {
        {"text", text},
        {"timestamp", std::chrono::duration_cast<std::chrono::seconds>(
                          std::chrono::system_clock::now().time_since_epoch() ).count()}
    };

    // Add metadata if provided
    if( !metadata.empty() ) {
        try {
            json meta = json::parse( metadata );
            for( auto &[key, value] : meta.items() ) {
                payload[key] = value;
            }
        } catch( ... ) {
            // If not valid JSON, store as string
            payload["metadata"] = metadata;
        }
    }

    json point = {
        {"id", generate_point_id()},
        {"vector", embedding},
        {"payload", payload}
    };

    json request = {
        {"points", json::array( {point} )}
    };

    std::string endpoint = "/collections/" + config_.collection_name + "/points";
    auto res = qdrant_client_.Put( endpoint, request.dump(), "application/json" );

    if( !res ) {
        std::cerr << "[LLMBridge] Failed to store memory: connection error" << std::endl;
        return false;
    }

    if( res->status != 200 ) {
        std::cerr << "[LLMBridge] Failed to store memory: HTTP " << res->status << std::endl;
        return false;
    }

    return true;
}

} // namespace cata_llm
