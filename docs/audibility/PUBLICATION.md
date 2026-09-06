# Audibility build source provenance

## Publication history boundary — audibility checkpoint

The main experimental checkout retains the separate local validation commit d40798c. Production fixes and the validated package were copied onto the previous public checkpoint in build/audibility-publication, branch codex/audibility-publication, for a clean fast-forward of github/codex/experimental-aeriform. The validation ancestor, harness/fixtures, status report and private feedback were explicitly verified absent from that publication tree.

Local production source commit 55aa009 maps to public production commit c06d1bb; local package/log commits 393f46a and bcdce70 map to f5e16fa and 55a245a. The complete Source tree is identical: 2d891c762f15c66e3c5b19d5c6972227e39ce309. Artifact bytes are also identical. Keep this intentional history separation when resuming: inspect file differences before merging remote commits, and do not publish the local validation ancestor.

## v3 EXP feature checkpoint

Local production source 2448ae80872e98fad9be4324f59cd248be9a7b85 maps to public source d2f966550ddff7d65647a4f7052d6e644d349fd6. The identical Source tree is d713d1831a7258740828e2129b2ac07f39564693. The v3 EXP EXE/VST3/ZIP and focused-check records are in artifacts/windows-x64-v3-exp. Broad acceptance remains with the separate testing agent. Local validation commits, including d40798c and the later cleanup notes, remain outside the public lineage. Continue publishing production-only commits via build/audibility-publication.
