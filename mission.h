enum mission_category {
 MISS_NULL = 0,
 MISS_GO_TO,	// GOTO considered harmful.
 MISS_FIND_ITEM, MISS_FIND_PERSON,
 MISS_ASSASSINATE,
 NUM_MISS_TYPES
};

struct mission {
 mission_category type;
 itype_id item_id;
 int count;
 int npc_id;
};
