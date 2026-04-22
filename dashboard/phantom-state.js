/**
 * phantom-state.js
 * Single source of truth for phantom selection and tissue parameters.
 * Must be loaded before app.js, brain-phantom.js, and brain-sim.js.
 */

(function () {
    // ── Default phantom definitions ───────────────────────────────────────
    // Tissue values from SpinSight constants.py (3T)
    window.PHANTOM_DEFS = {
        brain: {
            label: 'Brain',
            scanLabel: 'Brain Scan',
            tissues: [
                { name: 'gray',       label: 'Gray matter',  T1: 1450, T2: 100,  PD: 1.0,  color: '#00ff00', enabled: true },
                { name: 'white',      label: 'White matter', T1: 830,  T2: 69,   PD: 0.92, color: '#d40000', enabled: true },
                { name: 'CSF',        label: 'CSF',          T1: 4160, T2: 2100, PD: 1.0,  color: '#00ffff', enabled: true },
                { name: 'adipose',    label: 'Adipose',      T1: 370,  T2: 130,  PD: 1.0,  color: '#ffe680', enabled: true },
                { name: 'bonemarrow', label: 'Bone marrow',  T1: 898,  T2: 34,   PD: 1.0,  color: '#ffff44', enabled: true },
            ],
        },
    };

    function deepCopyTissues(id) {
        return window.PHANTOM_DEFS[id].tissues.map(t => Object.assign({}, t));
    }

    // ── Live state ────────────────────────────────────────────────────────
    window.PhantomState = {
        selectedPhantom: 'brain',
        tissues: deepCopyTissues('brain'),
    };

    // ── Actions ───────────────────────────────────────────────────────────
    window.selectPhantom = function (id) {
        if (!window.PHANTOM_DEFS[id]) return;
        window.PhantomState.selectedPhantom = id;
        window.PhantomState.tissues = deepCopyTissues(id);
        window.dispatchEvent(new CustomEvent('phantomChanged', { detail: { id } }));
    };

    window.resetTissues = function () {
        const id = window.PhantomState.selectedPhantom;
        window.PhantomState.tissues = deepCopyTissues(id);
        window.dispatchEvent(new CustomEvent('phantomChanged', { detail: { id, reset: true } }));
    };

    // ── Phantom tab UI ────────────────────────────────────────────────────
    function populatePhantomTable() {
        const tbody = document.getElementById('phantom-tissue-body');
        if (!tbody) return;
        const tissues = window.PhantomState.tissues;
        tbody.innerHTML = '';
        tissues.forEach((t, i) => {
            const row = document.createElement('tr');
            row.innerHTML = `
                <td>
                    <input type="checkbox" class="tissue-enable-cb" data-tissue="${i}" ${t.enabled !== false ? 'checked' : ''} title="Include in simulation">
                    <span class="tissue-color-swatch" style="background:${t.color};"></span>
                    ${t.label}
                </td>
                <td><input type="number" class="tissue-input" data-tissue="${i}" data-field="T1"
                           value="${t.T1}" min="1" max="10000" step="10"></td>
                <td><input type="number" class="tissue-input" data-tissue="${i}" data-field="T2"
                           value="${t.T2}" min="1" max="5000" step="1"></td>
                <td><input type="number" class="tissue-input" data-tissue="${i}" data-field="PD"
                           value="${t.PD}" min="0" max="1" step="0.01"></td>`;
            tbody.appendChild(row);
        });

        // Live update PhantomState on change
        tbody.querySelectorAll('.tissue-input').forEach(inp => {
            inp.addEventListener('change', () => {
                const i = parseInt(inp.dataset.tissue, 10);
                const field = inp.dataset.field;
                const val = parseFloat(inp.value);
                if (!isNaN(val)) {
                    window.PhantomState.tissues[i][field] = val;
                }
            });
        });

        // Enable/disable checkboxes
        tbody.querySelectorAll('.tissue-enable-cb').forEach(cb => {
            cb.addEventListener('change', () => {
                const i = parseInt(cb.dataset.tissue, 10);
                window.PhantomState.tissues[i].enabled = cb.checked;
                window.dispatchEvent(new CustomEvent('phantomChanged', { detail: { enabledChanged: true } }));
            });
        });
    }

    function updateScanTabLabel() {
        const id = window.PhantomState.selectedPhantom;
        const def = window.PHANTOM_DEFS[id];
        if (!def) return;
        const scanBtn = document.querySelector('.tab-btn[onclick*="brain-scan"]');
        if (scanBtn) {
            const match = scanBtn.textContent.match(/^(④\s*)/);
            const prefix = match ? match[1] : '';
            scanBtn.textContent = prefix + def.scanLabel;
        }
        const h2 = document.querySelector('#tab-brain-scan .brain-controls h2');
        if (h2) h2.textContent = def.scanLabel + ' Simulation';
    }

    document.addEventListener('DOMContentLoaded', () => {
        populatePhantomTable();
        updateScanTabLabel();

        const sel = document.getElementById('phantom-select');
        if (sel) {
            sel.addEventListener('change', () => {
                window.selectPhantom(sel.value);
                populatePhantomTable();
                updateScanTabLabel();
            });
        }

        const resetBtn = document.getElementById('phantom-reset-btn');
        if (resetBtn) {
            resetBtn.addEventListener('click', () => {
                window.resetTissues();
                populatePhantomTable();
            });
        }
    });
})();
