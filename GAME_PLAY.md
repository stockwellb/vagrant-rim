# Vagrant Rim — Play Mechanics & HUD Concepts (rough draft)

> Working design doc. Supersedes the original GOALS.md. Everything here is a
> draft to be refined; sections are tagged **[decided]** (we've committed to it),
> **[leaning]** (a recommendation not yet locked), or **[open]** (unresolved).

---

## 1. Premise

Vagrant Rim is a space-scavenger game with a collect-and-build economy. You pilot
a shuttle out of a mothership into faction-contested star systems, fight and
salvage for materials, and haul them home to craft ship and mothership upgrades
that let you push further into more dangerous territory. The core fantasy is the
**scavenger, not the ace** — a working pilot weighing risk against fuel and cargo.

**Aesthetic [decided]:** geometric / vector, in the lineage of *Battlezone* (1980)
and *Star Raiders* (1979). Procedural wireframe-ish geometry, near-zero art
pipeline. This is a look **and** a scope decision — it keeps a solo dev out of the
art mines.

---

## 2. Architecture: two layers

The whole game is **"instruments + a viewport"** — every screen is a HUD framing a
window onto space. There are two layers, and they never blur together: **[decided]**

- **Strategic layer** — menus and maps. Abstract, turn-free, no real-time flight.
  This is the mothership console and the shuttle's system map. Travel here is
  *abstract* (FTL transitions, fuel deducted per jump). The fuel-leash tension
  lives entirely in this layer.
- **Tactical layer** — the cockpit. Real-time combat and salvage inside a bounded
  instance at a single point of interest (POI). This is the only place you fly.

Keeping these separate makes each simpler than a single continuous-space design,
and it matches the existing HUD-heavy shell.

---

## 3. The mothership (console, never piloted) [decided]

The mothership is **home base as a UI**, not a place you fly. It presents three
functions:

1. **Launch** — undock the shuttle into the current system.
2. **Fabricate** — crafting: spend resources A/B/C on ship and mothership upgrades
   and on FTL capacity.
3. **Star Charts** — select which star system to travel to. Choosing one plays an
   FTL animation and relocates the mothership into that system.

The mothership relocates between *systems* via Star Charts (not in real time).
Whether it also repositions *within* a system is **[open]**.

---

## 4. Star systems as moddable maps [decided]

Each star system is a **Lua file in a `maps/` directory**, discovered at runtime.
The game lists however many map files it finds — this is the mod/DLC story for
free. C just scans the directory, loads and validates each file, and shows them.

**Hard rule:** every map carries a **stable string `id`**. Saves reference the id,
never the directory index — so adding or removing a map file never shifts or
corrupts existing saves.

A map describes the system as a set of bodies with ownership and POIs. Illustrative
schema (not final):

```lua
return {
  id   = "sol_frontier",       -- stable; saves reference this, never file order
  name = "Sol Frontier",
  difficulty = 2,              -- hand-set tier (emergent danger also comes from factions)
  sun  = { name = "Sol", hazard = "heat" },
  factions = { verge = { strength = 3 }, kesh = { strength = 1 } },  -- who's here, how strong
  bodies = {
    { id="terra", type="planet", orbit=1, owner="verge",
      moons = { { id="luna", type="moon", owner=nil, poi="wreckfield", resources={"A","B"} } } },
    { id="belt", type="field", orbit=2, owner="contested", poi="derelict", resources={"C"} },
  },
  buoys = { { id="b1", links={"terra","belt"} } },  -- cheap/free jumps between linked buoys
}
```

Bodies are **not** to-scale physics objects (a shuttle is orders of magnitude too
small for that). They serve at two registers: **navigation structure** on the
system map, and **atmosphere** (parallax backdrop) inside a tactical instance.

---

## 5. Factions & territory [decided]

Three factions, each holding territory. Territory is what makes the celestial
bodies meaningful:

- Every body has an **owner** (a faction / contested / neutral).
- A POI's **difficulty ≈ owner strength × how central it sits in their turf.**
- Rare resources live deep in faction cores, so *wanting rare materials is wanting
  to push into danger* — the same pressure as the fuel-leash. That coherence is
  the point.
- Each map varies **which** factions are present (1–3) and how strong — this is
  both the difficulty knob and the variety knob, all data-driven.

---

## 6. Resources, fuel, and the run

**Resources A/B/C [decided]:** three crafting materials, common → rare. Rarity is
tied to danger/depth (A everywhere; C only deep in faction territory or from tough
kills). Combined in recipes for upgrades and FTL capacity.

**Fuel [decided as master system]:** a separate resource, the master constraint.
Unlimited at the mothership; otherwise scavenged (enemy kills / wrecks drop fuel).
Fuel is spent **abstractly per FTL jump** in the strategic layer; FTL buoys give
cheap/free jumps between linked buoys. **[open]:** exact numbers, and whether
in-instance maneuvering also sips fuel.

**A "run" [decided in shape]:** undock → visit 2–3 POIs (fight / salvage) → return
to the mothership to offload and craft. The central decision every run is the
**turn-back gamble**: "I can still get home *now*, but there's a wreck one jump
deeper — do I risk it?" If that decision is tense, the game works.

**Death / stranding [decided]:** an emergency FTL fires on destruction *or*
stranding (fuel too low to get home) and drops you back at the mothership. It's a
**soft fail** — you lose the run's *cargo*, not your progress. This matches the
save design (slots = separate playthroughs, no save-scum). **[open]:** is the
abandoned cargo recoverable by flying back to where you died?

---

## 7. Tactical layer: the cockpit instance

The Battlezone model. **[decided]** unless noted.

**The sim is 2D.** Everything is `(x, y)` on a flat plane; your ship is
`(x, y, heading)`. There is no up/down — everyone is on the same "shipping lane."
The cockpit is a *camera* on this 2D truth, not a separate 3D world. This is the
single most important build decision (see §9).

- **Movement:** a vessel on a plane, with weight. **[leaning]** tank-with-inertia
  (rotate in place; translation has acceleration/momentum) over true-boat
  turn-radius — naval heft without boat helplessness.
- **Aiming:** **ship-fixed forward guns.** You point the hull at what you shoot, so
  aiming is welded to maneuvering and positioning is the core skill. No free
  reticle or turrets yet — they'd dilute the loop.
- **Radar:** a **360° "red-dot" radar** — necessary because the cockpit FOV can't
  cover your back. Because the sim is planar, the radar is *free*: it's just a
  top-down render of the same 2D data. Blips by type (red = hostiles, yellow/green
  = salvage, plus objective / jump-out point). Consider an edge-of-screen arrow to
  the nearest threat as well.
- **Instance bounds:** each POI is a **bounded place** with a size. Arrive → fight
  / scavenge → steer to a jump point (or hold-to-FTL) to leave. Soft boundary
  ("leaving sensor range, turn back") over an invisible wall.
- **Legibility:** the plane needs a **visible texture** — drifting dust/debris, a
  faint grid or ecliptic haze, and a big backdrop on the horizon (the planet
  you're orbiting, the sun). Pure black space makes speed and distance unreadable,
  which kills combat feel. This is a gameplay requirement, not decoration.
- **Salvage is a verb [decided]:** shoot a hulk → debris scatters on the plane →
  steer over it to tractor it in, ideally while still under fire. Never let looting
  collapse into a results screen — the collect-and-build economy is the heart of
  the game.
- **Hazards [leaning]:** sun proximity = heat / no-salvage zone; a planet's shadow =
  sensor cover. Keep these few and readable.

---

## 8. HUD concepts

**Mothership console:** three-tab instrument panel (Launch / Fabricate / Star
Charts). Reuses the existing raygui menu paradigm. Star Charts shows the current
system schematically — sun, orbital rings, bodies as icons, **faction territory as
colored fields**, buoys, known POIs; select a body to target.

**Shuttle system map (post-undock):** same schematic system view, now from the
shuttle, used to pick a POI to FTL into. Shows fuel cost per jump and buoy links.

**Cockpit HUD (tactical):** viewport onto the plane, framed by instruments:
- 360° red-dot radar
- heading / speed
- hull / shields
- fuel and cargo capacity
- weapon/heat state
- off-screen target indicator(s)
- salvage/tractor prompt

---

## 9. Rendering & build approach [decided]

Because the sim is 2D, **build the whole game in a cheap top-down debug view
first** — movement, combat, salvage, radar, faction encounters, all of it — then
layer the cockpit camera on top as a *rendering pass*. Gameplay progress is never
gated on the hard rendering work.

The cockpit is a **projection** of the 2D plane: relative bearing → horizontal
screen position, distance → object size + height toward the horizon (Battlezone's
exact trick). **[leaning]** this can be done in raylib 2D (2.5D projection) rather
than true 3D; revisit if the look demands real meshes.

**Save impact:** `GameSave` already has `player_x`/`player_y`; add a heading and a
current-system/POI reference (by id). Instance contents (mobs, debris) are
transient and not saved.

---

## 10. Open questions (biggest first)

1. **The long arc / win condition [open].** Endless upgrade treadmill? A fixed
   far-out destination gated by fuel + upgrades? A mothership you're rebuilding
   toward a goal? This is the biggest unresolved hole and it colors everything.
2. **Movement tuning [open]** — lock tank-with-inertia vs. true-boat, then values.
3. **Recoverable death cargo [open]** — can you fly back to a wreck of your own run?
4. **Fuel model numbers [open]** — cost per jump, buoy discounts, in-instance sip.
5. **Mothership intra-system mobility [open]** — does it reposition within a system?
6. **Crafting tree [open]** — concrete A/B/C recipes and the upgrade axes.
7. **2.5D projection vs. true 3D [open/leaning 2.5D]** for the cockpit.
