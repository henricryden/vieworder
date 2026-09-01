# Plan: Contrast Plot per Sequence

## Goal
Add two tissue selectors to the Phantom tab (②) and a contrast plot `C = 1 − |S₁/S₂|` (vs echo/excitation index) to each sequence sub-tab (③: MPRAGE, RARE, FSPGR, ENRAGE).

## Approach

### State (`phantom-state.js`)
- Add `contrastTissue1: 'gray'` and `contrastTissue2: 'white'` to `window.PhantomState`.
- Add `populateContrastSelects()` that builds/refreshes the two `<select>` elements (`#contrast-t1-select`, `#contrast-t2-select`) from the full tissue list. Binds `change` listeners that update PhantomState and dispatch a `contrastPairChanged` custom event.
- Call `populateContrastSelects()` from `populatePhantomTable()` so selects stay in sync whenever tissues change.

### Phantom tab HTML (`index.html`)
- Inside `phantom-table-panel`, after the Reset button, add a "Contrast pair" section with two selects and placeholder labels (`S₁` / `S₂`).

### Contrast chart containers (`index.html` — one per sequence tab)
Add an `mprage-plot-panel` row below the existing Mxy window chart in each sequence sub-tab:
- MPRAGE: `id="mprage-contrast-chart"` — after the MPRAGE window Mxy chart
- RARE: `id="rare-contrast-chart"` — after the echo train Mxy chart  
- FSPGR: `id="fspgr-contrast-chart"` — after the Full Scan Mxy chart
- ENRAGE: `id="enrage-contrast-chart"` — after the ENRAGE window Mxy chart

### Rendering (`index.html` inline `<script>`)
Add a shared helper:
```js
function renderContrastChart(containerId, mxyByTissue, xValues, contrastPair) {
    // mxyByTissue: { tissueName: Float32Array | number[] }
    // xValues: array of x-axis values (echo indices / excitation indices)
    // contrastPair: { t1Name, t2Name, t1Label, t2Label }
    // Computes: C[i] = 1 - |S1[i] / S2[i]|
    // Plots as a single-dataset D3SeqPlots line chart
}
```

Hook into each existing render function to call `renderContrastChart`:
- `renderMPRAGECharts()` — extract Mxy from `winResults` for both contrast tissues
- `renderRARECharts()` — extract Mxy from `echoResults` (acquired echoes only)
- `renderFSPGRCharts()` — call `simulateFSPGR` for both contrast tissues
- `renderENRAGECharts()` — extract Mxy from `winResults` for both contrast tissues

Also listen on `contrastPairChanged` in each tab's active state to re-render only the contrast chart.

## Steps
1. `phantom-state.js`: add default `contrastTissue1/2` to PhantomState; add `populateContrastSelects()` and call it from `populatePhantomTable()`.
2. `index.html` — phantom tab HTML: add "Contrast pair" selects section.
3. `index.html` — sequence tab HTML: add contrast chart divs (one per sequence).
4. `index.html` — add `renderContrastChart()` helper function.
5. `index.html` — hook `renderContrastChart()` into `renderMPRAGECharts`, `renderRARECharts`, `renderFSPGRCharts`, `renderENRAGECharts`.
6. `index.html` — listen for `contrastPairChanged` event to re-render contrast charts without re-running the full simulation.

## Out of Scope
- Changing the signal definition (always uses Mxy magnitude of the readout window/echo train).
- bSSFP (not yet implemented; shares the MPRAGE tab but has no simulation data yet).
- Contrast in the Full Sequence timeline charts (only the readout window / echo train).

## Open Questions
- Should the contrast chart show `1 − |S₁/S₂|` clamped to [−1, 1], or raw? (plan: raw, with natural y-axis)
    - Raw, make sure the y-axis scales correctly.
- If one of the selected tissues is disabled, should the contrast chart show an empty plot with a note, or be hidden? (plan: show a note inside the chart area)
    - Hidden.

