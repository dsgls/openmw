# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

OpenMW is a faithful open-source reimplementation of the Morrowind game engine, written in C++20. The project includes:
- **openmw**: The main game engine
- **opencs**: Construction Set editor (Qt-based)
- **launcher/wizard**: Installation and configuration tools
- Command-line utilities: bsatool, esmtool, navmeshtool, bulletobjecttool, essimporter, mwiniimporter, niftest

Version: 0.51.0 | License: GPLv3 | Lua API Revision: 109

## Building and Development Commands
All builds and tests should be performed in the build container (openmw.ubuntu).

### Building
```bash
# Create docker build container (only needs to happen once)
docker build -f Dockerfile.ubuntu -t openmw.ubuntu .

# Standard build
docker run -v ./:/openmw --rm openmw.ubuntu -DCMAKE_BUILD_TYPE=RelWithDebInfo

# Build for development (with tests and benchmarks)
docker run -v ./:/openmw --rm openmw.ubuntu \
  -DCMAKE_BUILD_TYPE=Debug \
  -DBUILD_COMPONENTS_TESTS=ON \
  -DBUILD_OPENMW_TESTS=ON \
  -DBUILD_OPENCS_TESTS=ON \
  -DBUILD_BENCHMARKS=ON

# Build specific targets
docker run -v ./:/openmw --rm openmw.ubuntu --build . --target openmw
docker run -v ./:/openmw --rm openmw.ubuntu --build . --target opencs
docker run -v ./:/openmw --rm openmw.ubuntu --build . --target components_tests
```

### Testing
```bash
# Run OpenMW tests (requires -DBUILD_OPENMW_TESTS=ON)
docker run -v ./:/openmw --rm --entrypoint sh openmw.ubuntu -c 'cd /openmw/build && ./openmw-tests'

# Run OpenMW-CS tests (requires -DBUILD_OPENCS_TESTS=ON)
docker run -v ./:/openmw --rm --entrypoint sh openmw.ubuntu -c 'cd /openmw/build && ./openmw-cs-tests'

# Run component tests (requires -DBUILD_COMPONENTS_TESTS=ON)
docker run -v ./:/openmw --rm --entrypoint sh openmw.ubuntu -c 'cd /openmw/build && ./components-tests'
```

### Code Quality
```bash
# Format code with clang-format (from repo root)
docker run -v ./:/openmw --rm --entrypoint sh openmw.ubuntu -c 'cd /openmw && clang-format -i <file>'

# Check formatting without modifying
docker run -v ./:/openmw --rm --entrypoint sh openmw.ubuntu -c 'cd /openmw && CI/check_clang_format.sh'

# Check CMake formatting
docker run -v ./:/openmw --rm --entrypoint sh openmw.ubuntu -c 'cd /openmw && CI/check_cmake_format.sh'

# Check file naming conventions
docker run -v ./:/openmw --rm --entrypoint sh openmw.ubuntu -c 'cd /openmw && CI/check_file_names.sh'
```

### Running OpenMW
```bash
# From build directory
./openmw

# With specific data directory
./openmw --data /path/to/morrowind/Data Files

# Skip main menu and start new game
./openmw --skip-menu --new-game

# Load specific save
./openmw --load-savegame /path/to/save.omwsave

# Enable Lua console scripts
./openmw --script-console
```

## High-Level Architecture

### Core Design Pattern: Environment Singleton
All engine subsystems are accessed through `MWBase::Environment` (apps/openmw/mwbase/environment.hpp), which provides centralized access to managers. This decouples subsystems while avoiding direct dependencies.

```cpp
// Typical usage pattern throughout the codebase
MWBase::Environment::get().getWorld()->enable(object);
MWBase::Environment::get().getSoundManager()->playSound(...);
```

### Main Subsystems (apps/openmw/mw*)

**mwworld/** - World State & Object Management
- `WorldImp`: Orchestrates physics, rendering, navigation, weather, time
- `ESMStore`: Central repository for all loaded ESM/ESP records (95+ types)
- `Scene`: Manages active cell streaming and lifecycle
- `CellStore`: Contains all objects in a cell
- Hierarchical storage: CellStore → ContainerStore → InventoryStore

**mwphysics/** - Physics Simulation
- Built on Bullet Physics with `BT_USE_DOUBLE_PRECISION` for large worlds
- `PhysicsSystem`: Multithreaded physics coordinator
- Handles actors, objects, projectiles, heightfield terrain

**mwrender/** - Rendering Pipeline
- Built on OpenSceneGraph (OSG)
- `RenderingManager`: Coordinates all rendering subsystems
- Separate managers for: Animation, Terrain, Water, Sky, Camera, Shadows, Effects
- Post-processing effects pipeline at components/fx/

**mwlua/** - Lua Scripting System
- **Critical**: Lua executes in a separate thread parallel to OSG Cull traversal
- Script contexts: Global (server-side logic) / Local (client-side, read-only) / Player / Menu
- **Threading rule**: Mutations affecting the scene graph MUST be queued via `LuaManager::addAction`
- Object types enforce permissions: `GObject` (mutable), `LObject` (read-only), `SelfObject` (local script's object)
- Event-based engine-to-script communication
- See apps/openmw/mwlua/README.md for detailed threading constraints

**mwscript/** - Original Morrowind Scripting
- Compiler and interpreter for MW script language
- Extensions organized by domain (AI, containers, dialogue, GUI, sound, stats, etc.)

**mwmechanics/** - Game Mechanics
- `MechanicsManager`: Coordinates AI, combat, magic, character stats
- AI packages: wander, follow, escort, combat, travel
- Spell effects, leveled list resolution, NPC schedules

**mwgui/** - User Interface
- Built on MyGUI library
- `WindowManager`: Manages 40+ specialized windows
- Layout XML files in files/mygui/
- Lua UI integration for custom mod content

**Frame Loop** (apps/openmw/engine.cpp:190-339):
```
Input → Lua Sync → State Update → Script Execution → Mechanics →
Physics → World Update → GUI → Rendering (parallel with Lua) → Resource Cleanup
```

### Shared Components (components/)

47 independent libraries used by both openmw and opencs:

**Data Loading:**
- `esm3/`: Morrowind ESM/ESP format (95+ record types)
- `esm4/`: Oblivion/Skyrim format support
- `bsa/`: Bethesda Archive reader
- `esmloader/`: Parallel ESM loading
- `nif/`: NetImmerse/Gamebryo mesh format parser
- `nifosg/`: NIF to OSG scene graph conversion

**Resource Management:**
- `resource/`: Unified caching system for scenes, images, keyframes, bullet shapes
- `vfs/`: Virtual File System (abstracts BSA archives + loose files)
- `sceneutil/`: OSG utilities (skeleton, shadows, lights, work queue)

**Navigation:**
- `detournavigator/`: Recast/Detour navmesh generation
  - Async updates in background threads
  - SQLite-based navmesh caching
  - Multi-agent pathfinding support

**Lua Integration:**
- `lua/`: LuaJIT wrapper with async package loading
- `lua_ui/`: Lua UI framework
- `l10n/`: YAML-based localization system

**Utilities:**
- `settings/`: Category-based settings (see openmw.readthedocs.io/reference/modding/settings/)
- `serialization/`: Binary serialization for save games
- `debug/`: Debug drawing and logging infrastructure

### OpenCS Architecture (apps/opencs/)

Qt-based editor using strict Model-View separation:
- `model/doc/`: Document management, operation stack (undo/redo)
- `model/world/`: Data model wrapping ESM records
- `model/tools/`: Validation and verification tools
- `view/`: Qt widgets and windows
- `view/render/`: 3D scene editing viewport

## Code Style and Conventions

### Formatting (enforced by .clang-format)
- **Standard**: C++20
- **Indentation**: 4 spaces, never tabs
- **Brace style**: Allman (braces on new lines)
- **Line length**: 120 characters max
- **Pointer alignment**: Left (`Type* ptr` not `Type *ptr`)
- **Namespace indentation**: All namespaces are indented
- **Include sorting**: Case-sensitive, grouped by priority

### CMake Patterns
- Use `add_openmw_dir()` macro for source file discovery (defined in cmake/OpenMWMacros.cmake)
- Build options follow `BUILD_<TARGET>` naming (e.g., `BUILD_OPENMW`, `BUILD_OPENCS`)
- Use `OPENMW_USE_SYSTEM_<LIB>` for system vs bundled library choice

### Common Patterns in Code

**Manager Pattern**: Each subsystem has a `*Manager` class (e.g., `SoundManager`, `MechanicsManager`)

**Store Pattern**: Type-safe collections with ID-based lookup
```cpp
const ESM::Spell* spell = MWBase::Environment::get().getWorld()
    ->getStore().get<ESM::Spell>().find(spellId);
```

**Resource Loading**: Always go through ResourceSystem/VFS, never direct file I/O
```cpp
auto node = mResourceSystem->getSceneManager()->getInstance(model);
```

**Lua Bindings**: Use sol2 library with type-safe object wrappers
```cpp
// Read-only for local scripts
LObjectMetatable["enabled"] = sol::readonly_property(isEnabled);
// Read-write for global scripts
GObjectMetatable["enabled"] = sol::property(isEnabled, setEnabled);
```

## Important Constraints

### Lua Threading
- Lua runs in parallel with OSG Cull traversal
- **Never** synchronously modify scene graph from Lua bindings
- **Always** queue scene-affecting changes: `context.mLuaManager->addAction([...] { ... });`
- Non-scene state can be mutated immediately (requires code inspection to determine)
- Queued changes visible to scripts only on next frame (use caching if immediate visibility needed)

### Original Engine Compatibility
OpenMW aims for faithful Morrowind behavior. Do not "fix" original engine quirks unless:
1. Fixed in official Morrowind patches
2. Severe limitation preventing future engine capabilities
3. Intent is obvious with minimal gameplay impact
4. Approved by project maintainers

See CONTRIBUTING.md "Guidelines for original engine fixes" section.

### Resource Lifecycle
- OSG objects must be deleted on main thread
- Use `UnrefQueue` for deferred deletion from worker threads
- Resources are cached with configurable expiry time
- `WorkQueue` for parallel resource loading

### Physics Precision
Bullet is configured with `BT_USE_DOUBLE_PRECISION`. Ensure new physics code respects this.

## Testing and CI

### Test Organization
- **apps/components_tests/**: Unit tests for 20+ component libraries (GoogleTest)
- **apps/openmw_tests/**: Integration tests for engine
- **apps/opencs_tests/**: Construction Set tests
- **apps/benchmarks/**: Performance benchmarks (Google Benchmark)
- **apps/niftest/**: NIF file validation tool

### CI Checks (GitLab)
- clang-format verification
- CMake format verification
- File naming convention checks
- Qt translation completeness
- Cross-platform builds (Linux, macOS, Windows, Android)
- Coverity static analysis

## Common Development Tasks

### Adding a New ESM Record Type
1. Define record structure in components/esm3/
2. Add load/save methods following existing record patterns
3. Register in components/esm/esmcommon.cpp
4. Add to ESMStore in components/esm/esmstore.hpp
5. Add tests in apps/components_tests/esm/

### Adding a Lua API Binding
1. Define API in files/lua_api/ using LDT documentation format
2. Implement binding in apps/openmw/mwlua/ using sol2
3. Choose appropriate object type (GObject/LObject/SelfObject) based on access needs
4. Queue scene-affecting mutations via `addAction`
5. Add tests in apps/components_tests/lua/
6. Update Lua API revision in CMakeLists.txt if breaking change

### Adding a New Component Library
1. Create directory under components/
2. Add CMakeLists.txt with `add_component_dir`
3. Include in components/CMakeLists.txt
4. Follow existing patterns for namespace and include guards
5. Add unit tests in apps/components_tests/

### Modifying the Frame Loop
Be extremely careful when changing engine.cpp frame sequencing:
- Physics must complete before rendering
- Lua sync must happen before script execution
- GUI updates need current world state
- Resource cleanup happens after rendering

## Documentation

- **User documentation**: https://openmw.readthedocs.io
- **Lua API reference**: Generated from files/lua_api/
- **Wiki**: https://wiki.openmw.org (developer reference, policies, standards)
- **Build documentation**: See docs/README.md for Sphinx setup

Generate documentation:
```bash
# Using Docker (recommended)
cd docs && docker build -t openmw_doc . && cd ..
docker run --user "$(id -u)":"$(id -g)" --volume "$PWD":/openmw openmw_doc \
    sphinx-build /openmw/docs/source /openmw/docs/build

# Without Docker
pip3 install -r docs/requirements.txt
sphinx-build docs/source docs/build
```

## Dependencies

**Required:**
- OpenSceneGraph (OSG) - 3D rendering
- Bullet Physics - collision and physics simulation
- MyGUI - UI framework
- SDL2 - input and window management
- FFmpeg - audio/video decoding
- LuaJIT - Lua scripting
- Recast & Detour - navigation mesh
- Qt5/Qt6 - for opencs, launcher, wizard
- Boost (filesystem, program_options, iostreams)

**Optional:**
- Google Test/Mock - testing
- Google Benchmark - benchmarks

Use `OPENMW_USE_SYSTEM_<LIB>=ON/OFF` to choose system vs bundled versions.

## Key Files to Know

- `apps/openmw/engine.cpp` - Main game loop and initialization
- `apps/openmw/mwbase/environment.hpp` - Central subsystem access point
- `apps/openmw/mwworld/worldimp.hpp` - World coordinator
- `apps/openmw/mwlua/luamanagerimp.hpp` - Lua execution manager
- `components/esm/esmstore.hpp` - Game data repository
- `components/resource/resourcesystem.hpp` - Asset management
- `components/detournavigator/navigator.hpp` - Pathfinding interface
- `CMakeLists.txt` - Root build configuration
- `.clang-format` - Code style rules

## Merge Request Guidelines

From CONTRIBUTING.md:
- Link to related bug report or discussion
- Summarize changes and motivation
- Describe testing performed
- Keep MRs focused (one feature/bugfix per MR)
- Use draft status for incomplete work
- Follow commit message format: reference issues as "Bug #123" or "Fixes #123"
- Rebase instead of merge when pulling from master
- Clean commit history (squash review fixups)
