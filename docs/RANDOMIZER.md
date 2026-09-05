# Seeded randomization, locks and undo

PLAY provides an editable 32-bit seed, New Seed, Randomize, Mutate, scope selection, Wild, Lock All/Unlock All and section locks. Right-click knobs, choice controls and toggles for individual locks. Knobs show a small teal lock marker.

Randomize produces a deterministic target from the seed. Each stable parameter ID consumes the same number of random draws regardless of locks, so locking one control does not reshuffle the others. Mutation uses a triangular signed offset within the normalized mutation radius around the starting patch; reproducing a mutation requires both the same seed and the same starting patch. Zero mutation is a no-op. The random stream is an explicit xorshift32 implementation rather than a library-dependent distribution.

Master output, MIDI/voice administration, quality and utility controls always remain unchanged. Additional silence-prone and external-input settings are locked by default. Normal mode favors central ranges, shorter attacks/releases and reduced feedback; Wild expands ranges while keeping feedback bounded. Original factory presets are preserved. One randomization/mutation is one undo action.

Seed, locks and snapshot data are stored in plugin state and exported user presets. Undo uses an explicit JUCE UndoManager command history instead of attaching the manager to APVTS, avoiding automation-driven history growth. Ctrl+Z undoes; Ctrl+Shift+Z and Ctrl+Y redo. GUI drags form one transaction. Host state restoration may involve ordinary float range-conversion rounding; regression tests compare restored normalized values within 1e-5.

Favorites use stable factory ordinals (factory order must remain append-only), UUIDs for new user presets, and parameter-content identifiers for legacy files. Saving an existing user file preserves its identifier, including across display-name changes. Favorites metadata lives in the user application-data EXP_Aeriform/favorites.xml, separately from factory source files. Missing identifiers remain in metadata. Experimental user presets live in Documents/EXP_Aeriform/Presets; import existing .aerpreset files through the browser.
