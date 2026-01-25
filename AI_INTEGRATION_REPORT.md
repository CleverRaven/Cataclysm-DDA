# CDDA AI Integration Report

## Overview

This report documents the comprehensive AI integration for Cataclysm: Dark Days Ahead (CDDA) using Ollama LLM and Qdrant vector database. The system enables dynamic world events, AI-generated quests, NPC-to-NPC interactions, and faction reputation systems.

## System Architecture

### Components

1. **LLM Bridge** (`src/llm_bridge.h`, `src/llm_bridge.cpp`)
   - HTTP communication with Ollama API
   - Vector embeddings via `nomic-embed-text` model
   - RAG pipeline using Qdrant vector database
   - LLM queries via `PetrosStav/gemma3-tools:4b` model

2. **AI World System** (`src/ai_world_system.h`, `src/ai_world_system.cpp`)
   - Dynamic world event generation
   - Faction relationship management
   - NPC-to-NPC interaction processing
   - Storyline tracking and progression

3. **AI Quest System** (`src/ai_quest_system.h`, `src/ai_quest_system.cpp`)
   - Quest generation from world events
   - Multiple objective types (FIND_ITEM, KILL_MONSTER, INVESTIGATE, etc.)
   - Quest rewards (items, cash, reputation, skills)
   - NPC quest delegation
   - Quest progress tracking

4. **AI Dialogue System** (`src/ai_dialogue.h`, `src/ai_dialogue.cpp`)
   - Context-aware NPC dialogue
   - Memory-augmented responses via RAG
   - Action parsing from LLM responses

## Configuration

```cpp
LLMConfig config;
config.ollama_host = "172.29.64.1";  // WSL host IP
config.ollama_port = 11434;
config.qdrant_host = "172.29.64.1";
config.qdrant_port = 6333;
config.llm_model = "PetrosStav/gemma3-tools:4b";
config.embed_model = "nomic-embed-text";
config.collection_name = "cdda_npc_memory";
config.vector_size = 768;
```

## Features Demonstrated

### 1. AI World Events

The system generates dynamic world events that affect multiple factions:

**Example Event (from screenshots/test_08_message_log_new_events.txt):**
```
>>> MAJOR WORLD EVENT: A cache of pre-war agricultural drones has been
unearthed in the abandoned agricultural research facility, 'Verdant Hope'.
The Hell's Raiders, seeking to establish a self-sufficient food source,
are attempting to seize control. The Tacoma Commune, reliant on scavenging
for sustenance, sees the drones as a threat to their established trade
routes. The Crimson Hand, a nomadic raider group known for their
technological expertise, has also taken an interest...

[OPPORTUNITY]: The player can choose to support either the Hell's Raiders
or the Tacoma Commune, attempt to negotiate a truce, or independently
secure the drones for themselves.

This affects the Hell's Raiders faction.
This affects the Tacoma Commune faction.
This affects the The Crimson Hand faction.
```

### 2. Quest Generation

Quests are automatically generated from world events:

**Example Quest (from screenshots/test_10_QUEST_GENERATED.txt):**
```
=== NEW QUEST AVAILABLE ===
Quest: Investigate the Disturbance
Objective: Investigate the area and report back.
Difficulty: 5/10 | Reward: $100 + 0 reputation
```

### 3. Storyline Progression

Events evolve over time, creating emergent narratives:

**Event 1:** Radio tower discovery with distress signal
**Event 2:** Temporal anomaly intensifies near "Sunken City"
**Event 3:** Factions compete for control of the anomaly

### 4. Faction System

- Real-time faction reputation tracking
- Inter-faction relationships affect world events
- Player actions impact faction standings

### 5. Quest Objective Types

```cpp
enum class QuestObjectiveType {
    FIND_ITEM,          // Find/obtain a specific item
    FIND_LOCATION,      // Reach a specific location
    KILL_MONSTER,       // Kill specific monster type
    TALK_TO_NPC,        // Talk to a specific NPC
    DELIVER_ITEM,       // Bring item to NPC
    INVESTIGATE,        // Go to location and investigate
    CRAFT_ITEM,         // Craft a specific item
    SURVIVE_TIME,       // Survive for X turns
    CUSTOM              // LLM-determined completion
};
```

### 6. Quest Rewards

```cpp
struct AIQuestReward {
    int faction_reputation = 0;
    int cash_value = 0;
    std::string item_id;
    int item_count = 1;
    std::string skill_id;
    int skill_xp = 0;
    std::string special_unlock;
    bool auto_reward = false;
};
```

## Technical Implementation

### API Integration

- **Ollama Embeddings:** `POST /api/embeddings`
- **Ollama Generation:** `POST /api/generate`
- **Qdrant Search:** `POST /collections/{name}/points/search`
- **Qdrant Store:** `PUT /collections/{name}/points`

### Game Loop Integration

The AI systems hook into the game's turn-based loop:

```cpp
void game::on_turn_update() {
    AIWorldSystem::instance().on_turn_update(calendar::turn);
    AIQuestSystem::instance().on_turn_update(calendar::turn);
}
```

### Memory/Context System

NPC memories and world state are stored in Qdrant for RAG retrieval:
- Player interactions stored with NPC context
- World events indexed for semantic search
- Past decisions influence future AI responses

## Files Modified/Created

| File | Status | Description |
|------|--------|-------------|
| `src/llm_bridge.h` | Created | LLM/RAG bridge header |
| `src/llm_bridge.cpp` | Created | LLM/RAG implementation |
| `src/ai_world_system.h` | Created | World system header |
| `src/ai_world_system.cpp` | Created | World system implementation |
| `src/ai_quest_system.h` | Created | Quest system header |
| `src/ai_quest_system.cpp` | Created | Quest system implementation |
| `src/ai_dialogue.h` | Created | Dialogue system header |
| `src/ai_dialogue.cpp` | Created | Dialogue system implementation |
| `src/ai_actions.h` | Created | Action executor header |
| `src/ai_actions.cpp` | Created | Action executor implementation |
| `tests/llm_bridge_test.cpp` | Created | Unit tests |

## Dependencies

- **cpp-httplib** (header-only): HTTP client
- **nlohmann/json** (header-only): JSON parsing
- **Ollama**: Local LLM server
- **Qdrant**: Vector database for RAG

## Screenshots

See `screenshots/` directory for in-game demonstrations:
- `test_04_game_view.txt` - Initial game load
- `test_06_message_log.txt` - AI world events in message log
- `test_08_message_log_new_events.txt` - Multiple events with faction impacts
- `test_10_QUEST_GENERATED.txt` - Quest generation confirmation
- `test_11_full_quest_log.txt` - Complete quest log with details

## Known Limitations

1. **Network Dependency**: Requires Ollama and Qdrant running
2. **Latency**: LLM calls are synchronous (may cause brief pauses)
3. **NPC-NPC Interactions**: Require multiple NPCs in same area to trigger
4. **Quest Completion**: Some objective types need further integration with game events

## Future Enhancements

1. Asynchronous LLM calls to prevent UI freezing
2. More diverse quest types with location integration
3. NPC-to-NPC dialogue visible to player when nearby
4. Quest chains with branching storylines
5. Persistent world state across game sessions

## Build Instructions

```bash
# Build with AI integration (terminal mode for faster compilation)
make TILES=0 SOUND=0 LOCALIZE=0 CCACHE=1 -j$(nproc)

# Run the game
./cataclysm
```

## Testing

```bash
# Build and run unit tests
make TILES=0 TESTS=1 CCACHE=1 -j$(nproc) tests
./tests/cata_test "[llm]" --rng-seed time
```

## Conclusion

The AI integration successfully demonstrates:
- Dynamic world event generation with rich, contextual content
- Automatic quest creation from world events
- Multi-faction storylines with player impact opportunities
- RAG-based memory system for persistent NPC interactions

The system is functional and generates meaningful, game-relevant content that enhances the emergent storytelling capabilities of CDDA.
