# CAD source files & export conventions

Source SolidWorks data lives under `mechanical/sw-models/`. Native `.SLDPRT` / `.SLDASM` / `.SLDDRW` files are **proprietary and binary** — they are the authoritative CAD, but the repo and tooling are much more useful when **neutral exports** sit next to (or under) that tree.

**Lock files:** SolidWorks creates `~$filename` lock files while a document is open. They are **ignored in git** (see `.gitignore`); do not add or share them.

---

## What to export (by type)

| Asset | Required exports | Optional / when useful |
|:------|:-----------------|:------------------------|
| **Custom part** (printed or machined) | **STEP** (`.step` or `.stp`) — same basename as the SolidWorks part, in the same folder or an `exports/` subfolder you keep consistent. | **PDF** from a **drawing** if the part has critical **dimensions, tolerances, or GD&T** (pins, bores, press-fits). **STL** (mesh) for slicer/FDM if you do not generate STL from the STEP elsewhere. |
| **Vendor / catalog part** (bearings, motors) | Prefer **one** canonical STEP in-repo if you rely on it for fit (e.g. `joint1-nema23.STEP`). For common hardware, a **link + key dimensions** in the BOM is often enough without duplicating every vendor model. | PDF datasheet or a **single** simplified solid if the vendor mesh is huge. |
| **Assembly** (how parts relate) | **STEP** of the **top-level assembly** (components can be include as separate parts per export options — choose what keeps file size and clarity reasonable). | **Exploded** view as **PDF** or a rendered image for the wiki/docs. **BOM** as **CSV** exported from SolidWorks when the assembly BOM is the source of truth. |
| **Drawing** (2D) | **PDF** for any sheet you want others (or this repo) to read without SolidWorks. | **DXF** of a **flat pattern** (sheet metal) or 2D profiles for waterjet/laser. |

---

## Naming and layout (suggested)

- Mirror names: e.g. `joint1-cycloidal-housing.SLDPRT` → `joint1-cycloidal-housing.step` (or a dated suffix only for milestones: `joint1-cycloidal-housing_r2.step`).
- If you add `mechanical/sw-models/exports/`, use the same basenames and document that layout here so `git status` stays predictable.
- For large motors or vendor dumps, one STEP per major component is enough; avoid checking in multiple revisions of the same part unless the team agrees on a retention policy.

---

## How this helps

- **STEP** preserves **geometry and assembly context** for FreeCAD, other CAE, and rough automated checks; text-based STEP is often **searchable** in tooling.
- **PDF drawings** carry **human-readable** dimensions and revision history for manufacture and design review.
- **STL** is for **slicers and collision meshes**; it is **not** a good substitute for toleranced drawings.

For collaboration with this workspace (e.g. AI-assisted review), **PDF + STEP** is the most useful pair for custom parts: PDF for the spec, STEP for the 3D truth.
