"""
build_levels.py - builds /Game/Maps/MainMenu and /Game/Maps/Facility for "Lockdown".

Run inside the Unreal Editor (UE 5.6):
    Tools > Execute Python Script...   or   Output Log:  py "<path>/build_levels.py"

Idempotent: existing maps / material instances are deleted or reused and rebuilt.

LAYOUT (cm, +X = forward through the facility, Y centred on 0, Z up, floor top = 0)
---------------------------------------------------------------------------------
Every section = Hazard corridor -> Security area -> Control room (-> gate).
Rooms are separated by 50cm thick bulkheads, each with a 400x400 doorway centred on Y=0.
Gates (ALevelGate) sit in the doorway at the end of each control room.

 Section 1 (easy)    X    0 ..  4450   hazard 0-1500, security 1550-3350, control 3400-4400, gate wall 4400-4450
 Section 2 (medium)  X 4450 .. 10400  hazard 4450-7250 (pit+platform, acid), security 7300-9300, control 9350-10350, gate wall 10350-10400
 Section 3 (final)   X 10400 .. 16950 hazard 10400-12200, big room 12250-15450, control 15500-16900, closing wall 16900-16950

ASSUMPTIONS about the C++ default meshes (adjust constants below if they differ):
 - Turret / Terminal / Gate / Pickup pivots sit on the floor (Z offset 0 / pedestal top).
 - Hazard zones are a unit 100cm box, so actor scale = size / 100.
"""
import math
import unreal

# --------------------------------------------------------------------------- constants
MAP_MENU = "/Game/Maps/MainMenu"
MAP_FACILITY = "/Game/Maps/Facility"
MAT_DIR = "/Game/Materials"
CUBE_PATH = "/Engine/BasicShapes/Cube.Cube"
BASIC_MAT_PATH = "/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"

T = 50.0               # wall / floor / ceiling thickness
CORRIDOR_W = 500.0
ROOM_H = 450.0         # interior floor-to-ceiling
DOOR_W = 400.0
DOOR_H = 400.0
FLOOR_T = T            # slab thickness below Z=0
CEIL_TOP = ROOM_H + T  # top of ceiling slab
PIT_DEPTH = 200.0
PEDESTAL_H = 100.0
PEDESTAL_SIZE = 160.0
COVER_LOW_H = 200.0    # tall enough to hide the 180cm player

EXPOSURE_MIN_EV = 0.0   # EV100 range for auto exposure (dark interior)
EXPOSURE_MAX_EV = 3.0
ACID_HALF_H = 40.0     # matches AAcidPuddle default box height
JAMMER_HALF_H = 150.0  # matches ASignalJammerZone default box height
HAZARD_H = 200.0
GATE_Z = 0.0
TERMINAL_Z = 0.0
PICKUP_Z = 70.0

LIGHT_STEP = 800.0
LIGHT_INTENSITY = 60.0   # candelas
LIGHT_ATTEN = 1200.0
LIGHT_COLOR = unreal.Color(200, 220, 255, 255)
RED_INTENSITY = 40.0     # candelas
RED_COLOR = unreal.Color(255, 20, 20, 255)

# Colours (linear)
COL_FLOOR = unreal.LinearColor(0.03, 0.03, 0.035, 1.0)
COL_WALL = unreal.LinearColor(0.10, 0.13, 0.18, 1.0)
COL_ACCENT = unreal.LinearColor(0.9, 0.65, 0.0, 1.0)

# Section definitions. All x offsets (dx) are relative to the start of the named room.
SECTIONS = [
    dict(
        id=1, haz_len=1500, sec_len=1800, sec_w=1400, ctl_len=1000, ctl_w=800, gate=True,
        hazards=[("acid", "haz", 800, -125, 300, 250)],
        cover=[(400, -250, (60, 400, COVER_LOW_H)), (600, 300, (200, 200, ROOM_H)),
               (900, 150, (60, 400, COVER_LOW_H)), (1100, -300, (200, 200, ROOM_H)),
               (1300, 250, (150, 150, 150)), (1350, 100, (150, 150, 150))],
        turrets=[("SMALL", 1650, -500, 160), ("SMALL", 1650, 500, 200)],
        red_lights=[(600, -600), (600, 600), (1500, -600), (1500, 600)],
        pickups=[("overcharge", "haz", 1350, 0), ("grenade", "sec", 300, -550), ("health", "ctl", 300, 250)],
        terminal=(800, 0),
    ),
    dict(
        id=2, haz_len=2800, sec_len=2000, sec_w=1600, ctl_len=1000, ctl_w=800, gate=True,
        pit=(400, 1400),           # pit spans haz-relative x 400..1400
        platform_y=0,
        hazards=[("acid", "haz", 1700, 0, 800, 500), ("jammer", "sec", 1000, 0, 600, 1560)],
        cover=[(400, -300, (200, 200, ROOM_H)), (400, 300, (200, 200, ROOM_H)),
               (700, -450, (60, 500, COVER_LOW_H)), (700, 450, (60, 500, COVER_LOW_H)),
               (1000, -200, (150, 150, 150)), (1000, 250, (150, 150, 150))],
        turrets=[("SMALL", 2700, 150, 180, "haz"),     # watches the pit and the acid
                 ("MEDIUM", 1500, -550, 160), ("MEDIUM", 1500, 550, 200), ("SMALL", 1100, 720, 215)],
        red_lights=[(500, -700), (500, 700), (1300, -700), (1300, 700)],
        pickups=[("grenade", "haz", 1550, 0), ("health", "sec", 150, -650), ("grenade", "sec", 300, 650),
                 ("health", "ctl", 300, -250)],
        terminal=(800, 0),
    ),
    dict(
        id=3, haz_len=1800, sec_len=3200, sec_w=2400, ctl_len=1400, ctl_w=1400, gate=False,
        hazards=[("acid", "haz", 600, -125, 300, 250), ("acid", "haz", 1100, 125, 300, 250)],
        cover=[(500, -500, (200, 200, ROOM_H)), (500, 500, (200, 200, ROOM_H)),
               (900, 0, (60, 600, COVER_LOW_H)), (1300, -800, (60, 500, COVER_LOW_H)),
               (1300, 800, (60, 500, COVER_LOW_H)), (1500, 0, (200, 200, ROOM_H)),
               (1900, -400, (150, 150, 150)), (1900, 400, (150, 150, 150)),
               (2100, -1000, (200, 200, ROOM_H)), (2100, 1000, (200, 200, ROOM_H))],
        turrets=[("LARGE", 2700, -800, 170), ("LARGE", 2700, 800, 190), ("MEDIUM", 2500, 0, 180),
                 ("SMALL", 1800, -1050, 150), ("SMALL", 1800, 1050, 210)],
        red_lights=[(500, -1000), (500, 1000), (1500, -1000), (1500, 1000), (2500, -1000), (2500, 1000)],
        pickups=[("overcharge", "haz", 1500, 0), ("health", "sec", 200, -1000), ("grenade", "sec", 200, 1000),
                 ("health", "ctl", 300, 0)],
        terminals=[(1100, -400), (1100, 400)],
    ),
]

# --------------------------------------------------------------------------- globals
actor_sub = None
level_sub = None
CUBE = None
MATS = {}
GATES = {}


# --------------------------------------------------------------------------- generic helpers
def vec(x, y, z):
    return unreal.Vector(x, y, z)


def rot(pitch=0.0, yaw=0.0, roll=0.0):
    return unreal.Rotator(roll=roll, pitch=pitch, yaw=yaw)


def load_cls(name):
    cls = unreal.load_class(None, "/Script/Lockdown." + name)
    if cls is None:
        raise RuntimeError("Could not load class /Script/Lockdown.%s - is the module compiled?" % name)
    return cls


def finish(actor, label, folder):
    actor.set_actor_label(label)
    actor.set_folder_path(folder)
    return actor


def spawn_class(cls, name, folder, location, rotation=None):
    actor = actor_sub.spawn_actor_from_class(cls, location, rotation or rot())
    return finish(actor, name, folder)


def spawn_box(name, center, size, material, folder="Geometry", rotation=None):
    """Cube StaticMeshActor. center & size in cm (the engine cube is 100cm, so scale = size/100)."""
    actor = actor_sub.spawn_actor_from_class(unreal.StaticMeshActor, center, rotation or rot())
    comp = actor.static_mesh_component
    comp.set_editor_property("static_mesh", CUBE)
    if material:
        comp.set_material(0, material)
    actor.set_actor_scale3d(vec(size[0] / 100.0, size[1] / 100.0, size[2] / 100.0))
    return finish(actor, name, folder)


def box_minmax(name, mn, mx, material, folder="Geometry"):
    center = vec((mn[0] + mx[0]) / 2, (mn[1] + mx[1]) / 2, (mn[2] + mx[2]) / 2)
    size = (mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2])
    return spawn_box(name, center, size, material, folder)


def ensure_material_instance(name, color, parent):
    path = "%s/%s" % (MAT_DIR, name)
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        mi = unreal.EditorAssetLibrary.load_asset(path)
    else:
        tools = unreal.AssetToolsHelpers.get_asset_tools()
        mi = tools.create_asset(name, MAT_DIR, unreal.MaterialInstanceConstant,
                                unreal.MaterialInstanceConstantFactoryNew())
    mi.set_editor_property("parent", parent)
    unreal.MaterialEditingLibrary.set_material_instance_vector_parameter_value(mi, "Color", color)
    unreal.MaterialEditingLibrary.update_material_instance(mi)
    unreal.EditorAssetLibrary.save_loaded_asset(mi)
    return mi


def setup_materials():
    global CUBE
    CUBE = unreal.load_asset(CUBE_PATH)
    parent = unreal.load_asset(BASIC_MAT_PATH)
    MATS["floor"] = ensure_material_instance("MI_Floor", COL_FLOOR, parent)
    MATS["wall"] = ensure_material_instance("MI_Wall", COL_WALL, parent)
    MATS["accent"] = ensure_material_instance("MI_Accent", COL_ACCENT, parent)


def clear_level():
    for a in actor_sub.get_all_level_actors():
        try:
            actor_sub.destroy_actor(a)
        except Exception:
            pass


def fresh_map(path):
    """Delete the map asset if present, then create a new empty one (and load it)."""
    unreal.EditorLoadingAndSavingUtils.new_blank_map(False)  # unload whatever is open
    if unreal.EditorAssetLibrary.does_asset_exist(path):
        unreal.EditorAssetLibrary.delete_asset(path)
    if not level_sub.new_level(path):
        raise RuntimeError("new_level failed for " + path)
    clear_level()


def set_game_mode(cls_name):
    world = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
    ws = world.get_world_settings()
    ws.set_editor_property("default_game_mode", load_cls(cls_name))


def make_light(cls, name, location, color, intensity, atten=None):
    light = actor_sub.spawn_actor_from_class(cls, location, rot())
    comp = light.get_component_by_class(unreal.LightComponent)
    comp.set_mobility(unreal.ComponentMobility.MOVABLE)
    comp.set_editor_property("intensity_units", unreal.LightUnits.CANDELAS)
    comp.set_editor_property("intensity", intensity)
    comp.set_editor_property("light_color", color)
    if atten is not None:
        comp.set_editor_property("attenuation_radius", atten)
    return finish(light, name, "Lighting")


# --------------------------------------------------------------------------- facility geometry
class Room(object):
    def __init__(self, tag, x0, length, width):
        self.tag, self.x0, self.length, self.width = tag, x0, length, width
        self.x1 = x0 + length          # interior end; the bulkhead occupies x1 .. x1+T


def bulkhead(name, x, wmax, door=True):
    """Wall slab occupying x..x+T spanning the widest neighbour, with an optional doorway at Y=0."""
    half = wmax / 2 + T
    zlo, zhi = -FLOOR_T, CEIL_TOP
    if not door:
        box_minmax(name, (x, -half, zlo), (x + T, half, zhi), MATS["wall"])
        return
    d = DOOR_W / 2
    box_minmax(name + "_L", (x, -half, zlo), (x + T, -d, zhi), MATS["wall"])
    box_minmax(name + "_R", (x, d, zlo), (x + T, half, zhi), MATS["wall"])
    box_minmax(name + "_Lintel", (x, -d, DOOR_H), (x + T, d, zhi), MATS["wall"])
    # yellow threshold strip so doorways read clearly
    box_minmax(name + "_Strip", (x, -d, 0), (x + T, d, 2), MATS["accent"])


def build_shell(room, pit=None):
    """Floor, ceiling and side walls for a room. `pit` = (a, b) room-relative x range with no floor."""
    w, x0, x1 = room.width, room.x0, room.x1 + T   # slabs run under the next bulkhead
    h = w / 2
    depth = FLOOR_T + (PIT_DEPTH if pit else 0)
    tag = room.tag
    if pit:
        a, b = x0 + pit[0], x0 + pit[1]
        box_minmax(tag + "_Floor_A", (x0, -h, -depth), (a, h, 0), MATS["floor"])
        box_minmax(tag + "_Floor_B", (b, -h, -depth), (x1, h, 0), MATS["floor"])
        box_minmax(tag + "_PitBottom", (a, -h, -depth), (b, h, -PIT_DEPTH), MATS["floor"])
        # ramp inside the pit so the player can climb out if they fall (600cm run, 200cm rise)
        run = 600.0
        ang = math.degrees(math.atan2(PIT_DEPTH, run))
        length = math.hypot(run, PIT_DEPTH)
        spawn_box(tag + "_PitRamp", vec(a + run / 2, 0, -PIT_DEPTH / 2 - 20),
                  (length, w - 100, 40), MATS["floor"], rotation=rot(pitch=-ang))
        for i, x in enumerate((a, b)):   # hazard strips on the pit lips
            box_minmax("%s_PitLip_%d" % (tag, i), (x - 60 if i == 0 else x, -h, 0),
                       (x if i == 0 else x + 60, h, 2), MATS["accent"])
    else:
        box_minmax(tag + "_Floor", (x0, -h, -depth), (x1, h, 0), MATS["floor"])
    box_minmax(tag + "_Ceiling", (x0, -h, ROOM_H), (x1, h, CEIL_TOP), MATS["floor"])
    box_minmax(tag + "_Wall_Left", (x0, -h - T, -depth), (x1, -h, CEIL_TOP), MATS["wall"])
    box_minmax(tag + "_Wall_Right", (x0, h, -depth), (x1, h + T, CEIL_TOP), MATS["wall"])


def build_ceiling_lights(room, sec_id):
    rows = 1 if room.width <= 800 else (2 if room.width <= 1600 else 3)
    xs = []
    x = room.x0 + LIGHT_STEP / 2
    while x < room.x1:
        xs.append(x)
        x += LIGHT_STEP
    n = 0
    for x in xs:
        for r in range(rows):
            y = room.width * (r + 0.5) / rows - room.width / 2
            n += 1
            make_light(unreal.PointLight, "Light_S%d_%s_%02d" % (sec_id, room.tag.split("_")[-1], n),
                       vec(x, y, ROOM_H - 30), LIGHT_COLOR, LIGHT_INTENSITY, LIGHT_ATTEN)


def hazard_stripes(room, count=3):
    """Yellow warning stripes across a hazard corridor entrance."""
    for i in range(count):
        x = room.x0 + 150 + i * 120
        box_minmax("%s_Stripe_%d" % (room.tag, i), (x, -room.width / 2, 0), (x + 50, room.width / 2, 2), MATS["accent"])


# --------------------------------------------------------------------------- gameplay actors
def spawn_pedestal(name, x, y, height=PEDESTAL_H):
    spawn_box(name, vec(x, y, height / 2), (PEDESTAL_SIZE, PEDESTAL_SIZE, height), MATS["wall"])


def spawn_turret(name, ttype, x, y, yaw):
    spawn_pedestal(name.replace("Turret", "Pedestal"), x, y)
    t = spawn_class(load_cls("Turret"), name, "Turrets", vec(x, y, PEDESTAL_H), rot(yaw=yaw))
    t.set_editor_property("turret_type", getattr(unreal.TurretType, ttype))
    return t


def spawn_hazard(kind, name, room, dx, dy, lx, ly):
    cls = load_cls("AcidPuddle" if kind == "acid" else "SignalJammerZone")
    half_h = ACID_HALF_H if kind == "acid" else JAMMER_HALF_H
    a = spawn_class(cls, name, "Hazards", vec(room.x0 + dx, dy, half_h))
    # Size the zone through its BoxExtent property so the trigger and visual stay in sync.
    a.set_editor_property("box_extent", vec(lx / 2, ly / 2, half_h))
    a.set_actor_location(a.get_actor_location(), False, False)  # re-runs OnConstruction
    return a


PICKUP_CLASSES = {"health": "HealthPickup", "overcharge": "OverchargePickup", "grenade": "GrenadeAmmoPickup"}


def spawn_pickup(name, kind, x, y):
    return spawn_class(load_cls(PICKUP_CLASSES[kind]), name, "Pickups", vec(x, y, PICKUP_Z))


def build_section(sec, x0):
    """Builds one section starting at x0 (hazard interior start). Returns the x where the next section begins."""
    sid = sec["id"]
    haz = Room("S%d_Hazard" % sid, x0, sec["haz_len"], CORRIDOR_W)
    scr = Room("S%d_Security" % sid, haz.x1 + T, sec["sec_len"], sec["sec_w"])
    ctl = Room("S%d_Control" % sid, scr.x1 + T, sec["ctl_len"], sec["ctl_w"])
    rooms = {"haz": haz, "sec": scr, "ctl": ctl}

    # --- shells and bulkheads
    build_shell(haz, pit=sec.get("pit"))
    build_shell(scr)
    build_shell(ctl)
    bulkhead("S%d_Bulkhead_HazSec" % sid, haz.x1, max(haz.width, scr.width))
    bulkhead("S%d_Bulkhead_SecCtl" % sid, scr.x1, max(scr.width, ctl.width))
    has_gate = sec["gate"]
    bulkhead("S%d_Bulkhead_End" % sid, ctl.x1, ctl.width, door=has_gate)
    if sid == 1:
        bulkhead("S1_Bulkhead_Start", x0 - T, haz.width, door=False)

    # --- lighting
    for r in (haz, scr, ctl):
        build_ceiling_lights(r, sid)
    for i, (dx, dy) in enumerate(sec["red_lights"], 1):
        make_light(unreal.PointLight, "Light_S%d_Red_%02d" % (sid, i), vec(scr.x0 + dx, dy, 300),
                   RED_COLOR, RED_INTENSITY, 900)

    hazard_stripes(haz)

    # --- cover
    for i, (dx, dy, size) in enumerate(sec["cover"], 1):
        spawn_box("Cover_S%d_%02d" % (sid, i), vec(scr.x0 + dx, dy, size[2] / 2), size, MATS["wall"])

    # --- hazards
    for i, (kind, rk, dx, dy, lx, ly) in enumerate(sec["hazards"], 1):
        spawn_hazard(kind, "%s_S%d_%02d" % ("Acid" if kind == "acid" else "Jammer", sid, i), rooms[rk], dx, dy, lx, ly)

    # --- pit moving platform (starts on the near lip, travels across to the far lip)
    if "pit" in sec:
        a, b = sec["pit"]
        plat = spawn_class(load_cls("MovingPlatform"), "Platform_S%d_01" % sid, "Platforms",
                           vec(haz.x0 + a + 150, sec["platform_y"], -10))
        plat.set_editor_property("target_offset", vec((b - a) - 300, 0, 0))

    # --- turrets
    turrets = []
    for i, t in enumerate(sec["turrets"], 1):
        ttype, dx, dy, yaw = t[:4]
        room = rooms[t[4]] if len(t) > 4 else scr
        turrets.append(spawn_turret("Turret_S%d_%02d" % (sid, i), ttype, room.x0 + dx, dy, yaw))

    # --- pickups
    for i, (kind, rk, dx, dy) in enumerate(sec["pickups"], 1):
        spawn_pickup("Pickup_%s_S%d_%02d" % (kind.capitalize(), sid, i), kind, rooms[rk].x0 + dx, dy)

    # --- gate + terminal(s)
    gate = None
    if has_gate:
        gate = spawn_class(load_cls("LevelGate"), "Gate_S%d" % sid, "Gates", vec(ctl.x1 + T / 2, 0, GATE_Z))
    term_pos = sec.get("terminals") or [sec["terminal"]]
    for i, (dx, dy) in enumerate(term_pos, 1):
        name = "Terminal_S%d" % sid if len(term_pos) == 1 else "Terminal_S%d_%02d" % (sid, i)
        term = spawn_class(load_cls("Terminal"), name, "Terminals", vec(ctl.x0 + dx, dy, TERMINAL_Z), rot(yaw=180))
        term.set_editor_property("controlled_turrets", turrets)
        if gate:
            term.set_editor_property("gate_to_open", gate)

    return ctl.x1 + T, (haz, scr, ctl)


# --------------------------------------------------------------------------- maps
def build_facility():
    fresh_map(MAP_FACILITY)
    setup_materials()

    x = 0.0
    bounds = []
    for sec in SECTIONS:
        start = x
        x, rooms = build_section(sec, x)
        bounds.append((sec["id"], start, x, rooms))

    spawn_class(unreal.PlayerStart, "PlayerStart_S1", "Geometry", vec(300, 0, 100))

    # --- global atmosphere
    fog = actor_sub.spawn_actor_from_class(unreal.ExponentialHeightFog, vec(0, 0, 200), rot())
    fog.get_component_by_class(unreal.ExponentialHeightFogComponent).set_editor_property("fog_density", 0.002)
    finish(fog, "Fog", "Lighting")

    sky = actor_sub.spawn_actor_from_class(unreal.SkyLight, vec(0, 0, 400), rot())
    sc = sky.get_component_by_class(unreal.SkyLightComponent)
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)
    sc.set_editor_property("intensity", 0.3)
    finish(sky, "SkyLight", "Lighting")

    ppv = actor_sub.spawn_actor_from_class(unreal.PostProcessVolume, vec(0, 0, 0), rot())
    ppv.set_editor_property("unbound", True)
    s = ppv.get_editor_property("settings")
    s.set_editor_property("override_vignette_intensity", True)
    s.set_editor_property("vignette_intensity", 0.4)
    s.set_editor_property("override_auto_exposure_min_brightness", True)
    s.set_editor_property("auto_exposure_min_brightness", EXPOSURE_MIN_EV)
    s.set_editor_property("override_auto_exposure_max_brightness", True)
    s.set_editor_property("auto_exposure_max_brightness", EXPOSURE_MAX_EV)
    ppv.set_editor_property("settings", s)
    finish(ppv, "PostProcess", "Lighting")

    set_game_mode("LockdownGameMode")
    level_sub.save_current_level()
    return bounds


def build_main_menu():
    fresh_map(MAP_MENU)
    setup_materials()

    # backdrop: floor slab plus a few monolithic props
    spawn_box("Menu_Floor", vec(600, 0, -25), (3000, 3000, 50), MATS["floor"])
    props = [(500, -250, 250, (200, 200, 500), 0), (700, 300, 150, (300, 150, 300), 20),
             (400, 600, 100, (150, 150, 200), -15), (900, -600, 350, (250, 250, 700), 10),
             (1100, 100, 200, (400, 100, 400), 0)]
    for i, (x, y, z, size, yaw) in enumerate(props, 1):
        spawn_box("Menu_Prop_%02d" % i, vec(x, y, z), size, MATS["wall"], rotation=rot(yaw=yaw))
    spawn_box("Menu_Accent", vec(1400, 0, 5), (50, 1200, 10), MATS["accent"])

    # dim moody lighting (no sky atmosphere so the backdrop stays dark)
    sun = actor_sub.spawn_actor_from_class(unreal.DirectionalLight, vec(0, 0, 800), rot(pitch=-35, yaw=140))
    sc = sun.get_component_by_class(unreal.DirectionalLightComponent)
    sc.set_mobility(unreal.ComponentMobility.MOVABLE)
    sc.set_editor_property("intensity", 0.8)
    sc.set_editor_property("light_color", unreal.Color(120, 150, 255, 255))
    finish(sun, "Light_Moon", "Lighting")
    make_light(unreal.PointLight, "Light_Red_01", vec(300, 400, 300), RED_COLOR, 8000, 1500)
    make_light(unreal.PointLight, "Light_Cold_01", vec(800, -500, 400), LIGHT_COLOR, LIGHT_INTENSITY, 1500)

    cam = spawn_class(unreal.CameraActor, "MenuCamera", "Geometry", vec(-500, -150, 250), rot(pitch=-8, yaw=10))
    try:
        cam.set_editor_property("auto_activate_for_player", unreal.AutoReceiveInput.PLAYER0)
    except Exception as exc:  # property name differs between engine versions
        unreal.log_warning("Could not set camera auto-activate: %s" % exc)

    set_game_mode("MainMenuGameMode")
    level_sub.save_current_level()


def main():
    global actor_sub, level_sub
    actor_sub = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
    level_sub = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)

    build_main_menu()
    bounds = build_facility()

    unreal.EditorAssetLibrary.save_directory("/Game/Maps")
    unreal.EditorAssetLibrary.save_directory(MAT_DIR)
    unreal.EditorLoadingAndSavingUtils.save_dirty_packages(True, True)

    unreal.log("=== Lockdown levels built ===")
    for sid, x0, x1, (h, s, c) in bounds:
        unreal.log("Section %d: X %d..%d | hazard %d-%d, security %d-%d, control %d-%d"
                   % (sid, x0, x1, h.x0, h.x1, s.x0, s.x1, c.x0, c.x1))
    unreal.log("Set in Project Settings > Maps & Modes (or DefaultEngine.ini):\n"
               "  EditorStartupMap / GameDefaultMap = /Game/Maps/MainMenu.MainMenu\n"
               "  Packaged builds: add both maps to 'List of Maps to Include in a Packaged Build'.\n"
               "  Facility is opened by MainMenuWidget via the 'Facility' map name.")


main()
