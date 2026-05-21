# Public media assets

README-linked visuals (committed for GitHub and local viewing).

| File | Source |
|------|--------|
| `joint-cad-exterior.png` | `image.png` — SolidWorks exterior render |
| `joint-cad-cross-section.png` | `image2.png` — SolidWorks cross-section |
| `joint-motion-demo.gif` | `20260516_220314_1_1.mp4` @ 22–28 s (8 fps, 280 px wide) |
| `preview.html` | Browser gallery when Cursor preview hides images |

## Viewing in Cursor

Cursor’s default markdown renderer often **does not show local images** (forum: [local images bug](https://forum.cursor.com/t/default-cursor-markdown-renderer-fails-to-display-local-remote-images-vs-code-markdown-preview-works/160156), [feature request](https://forum.cursor.com/t/show-images-in-markdown-preview-mode/151125)).

**Option A — VS Code-style preview (recommended)**

1. Open root `README.md`.
2. If you see a WYSIWYG/preview tab, right-click the tab → **Reopen Editor With…** → **Text Editor**.
3. `Ctrl+K V` (**Markdown: Open Preview to the Side**).

**Option B — Browser**

Open `public/preview.html` in Firefox/Chrome (from the file explorer or `xdg-open public/preview.html`).

This repo sets `"workbench.editorAssociations": { "*.md": "default" }` so `.md` files prefer the text editor.
