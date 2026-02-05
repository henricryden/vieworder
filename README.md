# K-Space View Ordering Dashboard

Interactive visualization of phase encoding view ordering algorithms for MRI k-space acquisition.

## Development

```bash
# Install dependencies
npm install

# Run local dev server
npm run dev
# Open http://localhost:8000/dashboard
```

## Build & Deploy

Build obfuscated, minified code for production:

```bash
# Build to ./dist/
npm run build
```

This produces:
- `dist/index.html` — main entry point
- `dist/index.single.html` — single-file offline version (all JS/CSS inlined)
- `dist/kspace-utils.js` — obfuscated coordinate generation (proprietary)
- `dist/d3-plots.js` — obfuscated plotting module
- `dist/app.js` — obfuscated application logic
- `dist/style.css` — minified CSS

### Deploy to GitHub Pages

**Option 1: Use `/docs` folder**
```bash
# Build
npm run build

# Copy dist contents to docs/
mkdir -p docs
cp -r dist/* docs/

# Commit and push
git add docs/
git commit -m "Build: production release"
git push origin main
```

Then in GitHub repository settings:
- Go to **Settings > Pages**
- Source: `main` branch, `/docs` folder
- Save

**Option 2: Use `gh-pages` branch**
```bash
# Install gh-pages package (optional, for automation)
npm install --save-dev gh-pages

# Build and deploy
npm run build
git add dist/
git commit -m "Build: production release"
git push origin main

# Manually push dist/ to gh-pages branch:
git subtree push --prefix dist origin gh-pages
```

Then in GitHub settings, set source to `gh-pages` branch.

**Site will be available at:** `https://<username>.github.io/<repo>/`

## Features

- **5 ordering algorithms**: Sequential, LCPO, CPLO, Chevron, CROC
- **Interactive controls**: ETL, center echo, acceleration, coverage, partial Fourier
- **Live visualization**: Phase encoding plans colored by shot or echo
- **Shot/Echo highlighting**: Select and highlight specific coordinates
- **Proprietary obfuscation**: JS implementation is minified and mangled for distribution

## Notes

- Development: Use `/dashboard` folder directly
- Production: Build to `/dist` before uploading to GitHub Pages
- Single-file HTML (`index.single.html`) works offline without a server
- CSS and HTML are preserved in dist/ for reference; only JS is heavily obfuscated
