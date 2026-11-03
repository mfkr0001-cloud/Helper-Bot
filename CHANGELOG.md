# Changelog

## v0.5.2 — Android build pipeline
- Added a GitHub Actions workflow that builds Android32 and Android64 separately and combines their `.geode` packages.
- Pinned the build to the Geode SDK version declared in `mod.json` (`v5.11.2`).
- Added an explicit error when the editor fails to create any generated objects instead of reporting a successful empty result.
- Updated project and mod metadata to v0.5.2.

## v0.5.1 — safer placement and mirrored layout
- Added MIRRORED layout with paired left/right motifs and mirrored detail rotations.
- Added conservative post-generation footprint fitting so approximate object extents stay inside the safe rectangle, not only their centres.
- Added a clear rejection for regions too small to fit the safety footprint.
- Added smoke tests for mirrored layout and footprint containment.
- Preserved the v0.5.0 style presets, floating panel, prompt parser, and editor createObject path.
- Preview overlay, exact sprite-bound validation, generated-batch tracking and safe remove/regenerate remain unfinished.


## v0.5.0 — style preset update
- Added the requested visible presets: FUTURISM, TECHNO, NEON, GOTHIC and NATURE.
- Kept all v0.4.0 style aliases and the existing floating editor panel.
- Routed each new preset to the existing procedural object families; no unverified object IDs were introduced.
- Updated AUTO prompt routing for city, technology, neon/space, nature/water and dark/ancient themes.
- Added smoke tests for all five requested presets and AUTO city detection.
- Kept real object placement through `LevelEditorLayer::createObject` and the normal editor Undo path.
- Preview overlay, generated-batch removal, duplicate-safe regeneration and custom region handles remain unfinished.

## v0.4.0
- Reworked placement from unrestricted random scattering into a procedural composition builder.
- Added a structured frame pass with thin rails and corner accents for sufficiently large selections.
- Added layout-specific motifs for BALANCED, CENTERED, ASYMMETRIC and RHYTHM compositions.
- Added spacing checks for filler details to reduce clusters of overlapping small objects.
- Added deterministic generation: the same seed, prompt-derived plan and selection produce the same result.
- Improved density-aware object caps and kept compact-selection filtering.
- Expanded the generation result report with detected theme, composition and density.
- Added tests for deterministic output and layout variation.

## v0.3.0
- Connected the editor's live selection (`m_selectedObjects` / `m_selectedObject`) to the decorator.
- Added selection bounds calculation through `LevelEditorLayer::getObjectRect` with a position fallback for zero-sized objects.
- Added real Geometry Dash decorative object placement through `LevelEditorLayer::createObject`.
- Added color, child-color, glow-color, rotation, scale and opacity assignment.
- Generated objects use the normal editor Undo history (`noUndo = false`).

## v0.2.0
- Added draggable and resizable editor-only floating panel, maximize/restore and saved geometry.
- Added local style, density, palette and composition settings.
- Expanded offline English/Russian theme detection and multi-layer plans.

## v0.1.0
- Initial Geode source project and editor-only launcher.
