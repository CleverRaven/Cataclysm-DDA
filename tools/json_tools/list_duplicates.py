#!/usr/bin/env python3
"""Lists duplicates in JSON by `type` and `id` fields
"""

from collections import defaultdict

from util import import_data


data = import_data()[0]
all_ids = defaultdict(set)
for obj in data:
    obj_id = obj.get('id')
    obj_type = obj.get('type')
    if obj_id and not isinstance(obj_id, list):
        if obj_id not in all_ids[obj_type]:
            all_ids[obj_type].add(obj_id)
        else:
            print(obj_type, obj_id)
