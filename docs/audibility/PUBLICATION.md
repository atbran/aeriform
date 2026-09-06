# Audibility build source provenance

## Publication history boundary — audibility checkpoint

The main experimental checkout retains the separate local validation commit d40798c. Production fixes and the validated package were copied onto the previous public checkpoint in build/audibility-publication, branch codex/audibility-publication, for a clean fast-forward of github/codex/experimental-aeriform. The validation ancestor, harness/fixtures, status report and private feedback were explicitly verified absent from that publication tree.

Local production source commit 55aa009 maps to public production commit c06d1bb; local package/log commits 393f46a and bcdce70 map to f5e16fa and 55a245a. The complete Source tree is identical: 2d891c762f15c66e3c5b19d5c6972227e39ce309. Artifact bytes are also identical. Keep this intentional history separation when resuming: inspect file differences before merging remote commits, and do not publish the local validation ancestor.
