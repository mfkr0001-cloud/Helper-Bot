# AI Decorator — roadmap

## 0.2.x — editor workspace
- [x] Editor-only floating panel
- [x] Drag and resize panel
- [x] Remember panel geometry
- [x] Local style and layout options
- [ ] Keyboard shortcut to show/hide the panel
- [ ] Optional left/right docking and snap-to-edge behavior

## 0.3.x — selection integration and first real generator
- [x] Read current selected-object collection
- [x] Compute selection bounds in editor coordinates
- [x] Create decorative objects through `LevelEditorLayer::createObject`
- [x] Apply palette, scale, rotation and opacity
- [x] Use normal editor Undo history
- [ ] Verify sprite visual bounds and save/reopen behavior in-game

## 0.4.x — procedural composition
- [x] Structured frame pass
- [x] Layout-specific motifs: balanced, centered, asymmetric and rhythm
- [x] Spacing checks for filler details
- [x] Deterministic seed-based generation
- [ ] Add optional preview overlay before committing
- [ ] Validate all object IDs and sprite bounds in Geometry Dash 2.2081

## 0.5.2 — Android build pipeline
- [x] Add GitHub Actions Android32 and Android64 build matrix
- [x] Combine platform builds into one `.geode` artifact
- [x] Report object-creation failure instead of false success
- [ ] Confirm CI build and test on a real Geometry Dash 2.2081 installation

## 0.5.1 — safer placement and mirrored layout
- [x] Add MIRRORED composition with paired motifs
- [x] Constrain approximate object footprints to the safe rectangle
- [x] Reject regions too small for safe placement
- [x] Add smoke tests for mirrored layout and footprint containment
- [ ] Validate exact object sprite bounds in Geometry Dash 2.2081
- [ ] Preview overlay before committing
- [ ] Track generated objects as a batch for safe remove/regenerate

## 0.5.0 — style presets
- [x] Add FUTURISM, TECHNO, NEON, GOTHIC and NATURE presets
- [x] Preserve v0.4.0 aliases and real editor object placement
- [x] Add smoke tests for requested presets and AUTO city detection
- [ ] Preview overlay before committing
- [ ] Track generated objects as a batch for safe remove/regenerate
- [ ] Regenerate with a new seed without stacking duplicates
- [ ] Add manual object count and seed controls
- [ ] Add object-category filters and custom palettes
- [ ] Add a dedicated draggable selection rectangle independent of existing selected objects

## 1.0.0 — stable release criteria
- [ ] Build and test Windows x64 and Android 32/64-bit with Geode SDK
- [ ] Verify selection detection on each target platform
- [ ] Verify generated objects stay visually inside the selection
- [ ] Verify Undo/Redo restores the level exactly
- [ ] Verify saved levels reopen without missing or shifted objects
