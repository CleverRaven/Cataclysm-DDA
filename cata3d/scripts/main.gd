extends Node3D

# External first-person presentation client for the live CDDA simulation.
# CDDA owns all gameplay state. This client only renders snapshots and sends actions.

const STATE_PORT := 7778
const COMMAND_PORT := 7777
const LEVEL_HEIGHT := 3.0
const WALL_HEIGHT := 2.5
const PLAYER_EYE_HEIGHT := 1.65

var state_udp := PacketPeerUDP.new()
var command_udp := PacketPeerUDP.new()
var world_root := Node3D.new()
var camera_rig := Node3D.new()
var camera := Camera3D.new()
var overlay := Label.new()
var cells_root := Node3D.new()
var entities_root := Node3D.new()
var floors_root := Node3D.new()
var colliders_root := Node3D.new()

var yaw := 0.0
var pitch := 0.0
var mouse_locked := true
var last_turn := -1

func _ready() -> void:
    add_child(world_root)
    world_root.add_child(floors_root)
    world_root.add_child(cells_root)
    world_root.add_child(entities_root)
    world_root.add_child(colliders_root)

    add_child(camera_rig)
    camera_rig.add_child(camera)
    camera.position = Vector3(0, PLAYER_EYE_HEIGHT, 0)
    camera.current = true
    camera.fov = 78.0

    overlay.position = Vector2(18, 18)
    overlay.add_theme_font_size_override("font_size", 18)
    overlay.text = "Cata3D: waiting for live CDDA bridge on UDP 7778..."
    add_child(overlay)

    var err := state_udp.bind(STATE_PORT, "127.0.0.1")
    if err != OK:
        overlay.text = "UDP 7778 error: %s" % err

    command_udp.set_dest_address("127.0.0.1", COMMAND_PORT)
    Input.mouse_mode = Input.MOUSE_MODE_CAPTURED
    _send_action("REQUEST_STATE")

func _process(_delta: float) -> void:
    _read_state_packets()

func _read_state_packets() -> void:
    while state_udp.get_available_packet_count() > 0:
        var packet := state_udp.get_packet()
        var parsed = JSON.parse_string(packet.get_string_from_utf8())
        if parsed is Dictionary:
            _apply_snapshot(parsed)

func _apply_snapshot(state: Dictionary) -> void:
    last_turn = int(state.get("turn", 0))

    # Snapshot coordinates are relative to the CDDA player.
    # Therefore the player is always at the origin of our local 3D scene.
    camera_rig.position = Vector3.ZERO

    _clear_world()

    for c in state.get("cells", []):
        if c.size() >= 7:
            _build_cell(int(c[0]), int(c[1]), int(c[2]),
                str(c[3]), str(c[4]), int(c[5]) != 0, int(c[6]) != 0)

    for item_data in state.get("items", []):
        if item_data.size() >= 4:
            _build_ground_sprite(int(item_data[0]), int(item_data[1]),
                int(item_data[2]), str(item_data[3]))

    var monsters: Array = state.get("monsters", [])
    for i in monsters.size():
        var m = monsters[i]
        if m.size() >= 4:
            _build_monster(i, int(m[0]), int(m[1]), int(m[2]), str(m[3]))

    overlay.text = "Cata3D LIVE  |  CDDA turn %d  |  WASD = CDDA move  |  mouse = look  |  E = interact" % last_turn

func _clear_world() -> void:
    for node in cells_root.get_children():
        node.queue_free()
    for node in floors_root.get_children():
        node.queue_free()
    for node in entities_root.get_children():
        node.queue_free()
    for node in colliders_root.get_children():
        node.queue_free()

func _cell_position(dx: int, dy: int, dz: int) -> Vector3:
    return Vector3(float(dx), float(dz) * LEVEL_HEIGHT, float(dy))

func _make_material(color: Color) -> StandardMaterial3D:
    var mat := StandardMaterial3D.new()
    mat.albedo_color = color
    mat.roughness = 0.95
    return mat

func _make_box(size: Vector3, color: Color) -> MeshInstance3D:
    var mesh := BoxMesh.new()
    mesh.size = size
    var node := MeshInstance3D.new()
    node.mesh = mesh
    node.material_override = _make_material(color)
    return node

func _build_cell(dx: int, dy: int, dz: int, terrain: String, furniture: String,
        impassable: bool, _outside: bool) -> void:
    var pos := _cell_position(dx, dy, dz)

    var floor := _make_box(Vector3(1.0, 0.12, 1.0), _terrain_color(terrain))
    floor.position = pos + Vector3(0, -0.06, 0)
    cells_root.add_child(floor)

    if impassable or _is_solid_terrain(terrain):
        var wall := _make_box(Vector3(1.0, WALL_HEIGHT, 1.0),
            _terrain_wall_color(terrain))
        wall.position = pos + Vector3(0, WALL_HEIGHT * 0.5, 0)
        cells_root.add_child(wall)
        _add_box_collision(wall.position, Vector3(1, WALL_HEIGHT, 1))
        return

    if furniture != "f_null" and furniture != "null" and furniture != "":
        var height := 1.0
        var f := furniture.to_lower()
        if f.contains("counter") or f.contains("table"):
            height = 0.8
        elif f.contains("bed"):
            height = 0.55
        elif f.contains("chair"):
            height = 0.9

        var box := _make_box(Vector3(0.82, height, 0.82), Color(0.38, 0.28, 0.20))
        box.position = pos + Vector3(0, height * 0.5, 0)
        cells_root.add_child(box)
        _add_box_collision(box.position, Vector3(0.82, height, 0.82))

func _add_box_collision(pos: Vector3, size: Vector3) -> void:
    var body := StaticBody3D.new()
    body.position = pos
    var shape := BoxShape3D.new()
    shape.size = size
    var collision := CollisionShape3D.new()
    collision.shape = shape
    body.add_child(collision)
    colliders_root.add_child(body)

func _build_ground_sprite(dx: int, dy: int, dz: int, item_id: String) -> void:
    var quad := QuadMesh.new()
    quad.size = Vector2(0.38, 0.38)
    var node := MeshInstance3D.new()
    node.mesh = quad
    node.position = _cell_position(dx, dy, dz) + Vector3(0, 0.012, 0)
    node.rotation.x = -PI * 0.5
    node.material_override = _make_material(_item_color(item_id))
    entities_root.add_child(node)

func _build_monster(index: int, dx: int, dy: int, dz: int, monster_id: String) -> void:
    var quad := QuadMesh.new()
    quad.size = Vector2(0.72, 1.55)
    var node := MeshInstance3D.new()
    node.mesh = quad
    node.position = _cell_position(dx, dy, dz) + Vector3(0, 0.78, 0)
    node.material_override = _make_material(_monster_color(monster_id))
    node.cast_shadow = GeometryInstance3D.SHADOW_CASTING_SETTING_OFF
    entities_root.add_child(node)

func _is_solid_terrain(id: String) -> bool:
    var s := id.to_lower()
    return s.contains("wall") or s.contains("door_c") or s.contains("window") or s.contains("bars")

func _terrain_color(id: String) -> Color:
    var s := id.to_lower()
    if s.contains("road") or s.contains("pavement") or s.contains("asphalt"):
        return Color(0.22, 0.22, 0.22)
    if s.contains("grass") or s.contains("dirt"):
        return Color(0.24, 0.34, 0.20)
    if s.contains("water"):
        return Color(0.16, 0.30, 0.42)
    if s.contains("wood") or s.contains("floor"):
        return Color(0.34, 0.28, 0.20)
    if s.contains("concrete") or s.contains("cement"):
        return Color(0.40, 0.40, 0.40)
    return Color(0.32, 0.32, 0.32)

func _terrain_wall_color(id: String) -> Color:
    var s := id.to_lower()
    if s.contains("brick"):
        return Color(0.42, 0.25, 0.20)
    if s.contains("wood"):
        return Color(0.36, 0.24, 0.14)
    return Color(0.27, 0.27, 0.28)

func _item_color(id: String) -> Color:
    var s := id.to_lower()
    if s.contains("knife") or s.contains("sword"):
        return Color(0.72, 0.74, 0.78)
    if s.contains("food") or s.contains("apple") or s.contains("meat"):
        return Color(0.55, 0.24, 0.20)
    if s.contains("bottle") or s.contains("water"):
        return Color(0.20, 0.55, 0.72)
    return Color(0.70, 0.52, 0.22)

func _monster_color(id: String) -> Color:
    if id.to_lower().contains("zombie"):
        return Color(0.42, 0.62, 0.40)
    if id.to_lower().contains("dog"):
        return Color(0.54, 0.34, 0.20)
    return Color(0.55, 0.55, 0.55)

func _unhandled_input(event: InputEvent) -> void:
    if event is InputEventMouseMotion and mouse_locked:
        yaw -= event.relative.x * 0.003
        pitch = clamp(pitch - event.relative.y * 0.003, -1.35, 1.35)
        camera_rig.rotation.y = yaw
        camera.rotation.x = pitch
        return

    if event is InputEventKey and event.pressed and not event.echo:
        var key := event as InputEventKey
        match key.physical_keycode:
            KEY_W:
                _send_relative_move(0)
            KEY_D:
                _send_relative_move(2)
            KEY_S:
                _send_relative_move(4)
            KEY_A:
                _send_relative_move(6)
            KEY_E:
                _send_action("INTERACT")
            KEY_F:
                _send_action("FIRE")
            KEY_G:
                _send_action("PICKUP")
            KEY_I:
                _send_action("INVENTORY")
            KEY_Q:
                _send_action("CLOSE")
            KEY_C:
                _send_action("OPEN")
            KEY_PAGEUP:
                _send_action("MOVE_UP")
            KEY_PAGEDOWN:
                _send_action("MOVE_DOWN")
            KEY_ESCAPE:
                mouse_locked = not mouse_locked
                Input.mouse_mode = Input.MOUSE_MODE_CAPTURED if mouse_locked else Input.MOUSE_MODE_VISIBLE

func _send_relative_move(local_sector: int) -> void:
    var sector := int(round(fposmod(yaw, TAU) / (PI / 4.0))) % 8
    var world_sector := (sector + local_sector) % 8
    var commands := ["MOVE_FWD", "MOVE_FWD_RIGHT", "MOVE_RIGHT", "MOVE_BACK_RIGHT",
        "MOVE_BACK", "MOVE_BACK_LEFT", "MOVE_LEFT", "MOVE_FWD_LEFT"]
    _send_action(commands[world_sector])

func _send_action(action_name: String) -> void:
    command_udp.put_packet(action_name.to_utf8_buffer())
