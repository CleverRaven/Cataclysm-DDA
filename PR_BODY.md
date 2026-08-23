#### Summary
Mods "Aftershock: the chirurgic suite (super autodoc) can now spawn in augmentation clinics"

#### Purpose of change

Fixes #47814

Aftershock defines a top-tier autodoc, `f_autodoc_super` ("chirurgic suite", `surgery_skill_multiplier: 999999`), in `data/mods/aftershock_exoplanet/maps/furniture.json`, but no mapgen or palette in the mod ever places it — so it cannot spawn anywhere. Verified against current master: the only references to `f_autodoc_super` outside its definition are in the unrelated Isolation-Protocol mod.

#### Describe the solution

Give the third-floor private surgery room of the augmentation clinic (`afs_augmentation_clinic_n3`) a 1-in-4 chance to have a chirurgic suite instead of a standard Autodoc Mk. XVI, using a weighted furniture distribution:

```json
"?": [ [ "f_autodoc", 3 ], [ "f_autodoc_super", 1 ] ]
```

Rationale for the placement: the mod already has a clear autodoc tier ladder — `f_autodoc_cheap` in the back-alley bionic basement, `f_autodoc` in the clinic and Port Augustmoon — and the top-floor private surgery of a clinic you have to fight through is the natural (and balance-conservative) home for the top tier, rather than a guaranteed spawn in the safe Port Augustmoon hub. Happy to tune the weight if maintainers prefer rarer/more common.

#### Describe alternatives you've considered

- Deterministically replacing the n3 autodoc — makes every clinic carry a guaranteed easy-install suite; felt too generous.
- Placing it in Port Augustmoon's med bay — a guaranteed, zero-risk, easy-install autodoc in the starting hub would trivialize bionic installation.
- Adding it to the secure bionic storage room on n2 — viable, but the n3 private surgery needed only a one-line change.

#### Testing

- `tools/format/json_formatter.cgi` over the changed file: no formatting changes beyond the edit.
- `tests/cata_test --mods=aftershock_exoplanet` (mod data loads clean).

#### Additional context

The weighted-distribution syntax matches existing core usage, e.g. `data/json/mapgen/homeimprovement_superstore_new.json`.
