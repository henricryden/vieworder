const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const test = require('node:test');
const vm = require('node:vm');

function loadKSpaceUtils() {
    const file = path.join(__dirname, '..', 'dashboard', 'kspace-utils.js');
    const code = fs.readFileSync(file, 'utf8') + '\nthis.KSpaceUtils = KSpaceUtils;';
    const context = { console: { log() {} } };
    vm.createContext(context);
    vm.runInContext(code, context);
    return context.KSpaceUtils;
}

function makeCoords(KSpaceUtils) {
    return KSpaceUtils.generateCoordinates(220, 160, 2, 2, true, 'elliptical', 32, 1.0);
}

function signature(coords) {
    return coords
        .map((c) => `${c.ky},${c.kz}:${c.shot}:${c.echo}`)
        .sort();
}

function assertUniqueShotEcho(coords) {
    const slots = new Set();
    for (const coord of coords) {
        assert.ok(Number.isInteger(coord.shot) && coord.shot >= 0, 'shot is assigned');
        assert.ok(Number.isInteger(coord.echo) && coord.echo >= 1, 'echo is assigned');
        const key = `${coord.shot}:${coord.echo}`;
        assert.equal(slots.has(key), false, `duplicate shot/echo slot ${key}`);
        slots.add(key);
    }
}

function assertFiniteRms(metrics) {
    assert.ok(metrics);
    assert.ok(Number.isFinite(metrics.rmsJump), 'RMS jump is finite');
    assert.ok(metrics.rmsJump >= 0, 'RMS jump is non-negative');
}

function assertDirectMacroConstraints(coords, maxMacroWidth, etl) {
    const widthsByMacro = new Map();
    for (const coord of coords) {
        assert.ok(Number.isInteger(coord.macroEcho) && coord.macroEcho >= 1, 'macro echo is assigned');
        assert.ok(Number.isInteger(coord.macroStartEcho) && coord.macroStartEcho >= 1, 'macro start is assigned');
        assert.ok(Number.isInteger(coord.macroEndEcho) && coord.macroEndEcho <= etl, 'macro end is assigned');
        const startEcho = coord.macroStartEcho;
        const endEcho = coord.macroEndEcho;
        assert.ok(coord.echo >= startEcho && coord.echo <= endEcho, 'echo stays inside direct macro segment');
        assert.ok(coord.macroWidth >= 1 && coord.macroWidth <= maxMacroWidth, 'macro segment width is bounded');
        widthsByMacro.set(coord.macroEcho, coord.macroWidth);
    }

    if (maxMacroWidth > 2) {
        let previousWidth = 0;
        for (const macroEcho of [...widthsByMacro.keys()].sort((a, b) => a - b)) {
            const width = widthsByMacro.get(macroEcho);
            if (width < previousWidth) {
                assert.equal(macroEcho, Math.max(...widthsByMacro.keys()), 'only the final clipped segment may shrink');
            }
            previousWidth = width;
        }
    }
}

function countShotChanges(baseCoords, macroCoords) {
    const baseShots = new Map(baseCoords.map((coord) => [`${coord.ky},${coord.kz}`, coord.shot]));
    let changes = 0;
    for (const coord of macroCoords) {
        if (baseShots.get(`${coord.ky},${coord.kz}`) !== coord.shot) changes++;
    }
    return changes;
}

test('macro width 1 preserves the original view-order assignment', () => {
    const KSpaceUtils = loadKSpaceUtils();
    const first = makeCoords(KSpaceUtils);
    KSpaceUtils.assignViewOrdering(first, 220, 'chevron', 105, 'ky', null, { macroWidth: 1 });

    const second = makeCoords(KSpaceUtils);
    KSpaceUtils.assignViewOrdering(second, 220, 'chevron', 105, 'ky', null, { macroWidth: 1 });

    assert.deepEqual(signature(second), signature(first));
    assertUniqueShotEcho(second);
    assertFiniteRms(second.jumpMetrics);
    for (const coord of second) {
        assert.equal(coord.baseEcho, coord.echo);
        assert.equal(coord.baseShot, coord.shot);
    }
});

test('ramped coarse-to-fine macro echoes assign unique slots inside macro segments', () => {
    const KSpaceUtils = loadKSpaceUtils();
    const coords = makeCoords(KSpaceUtils);
    KSpaceUtils.assignViewOrdering(coords, 220, 'croc', 105, 'ky', null, { macroWidth: 10 });

    assertUniqueShotEcho(coords);
    assertDirectMacroConstraints(coords, 10, 220);
    assertFiniteRms(coords.jumpMetrics);
});

test('coarse-to-fine macro echoes can change shot membership for nontrivial width', () => {
    const KSpaceUtils = loadKSpaceUtils();
    const base = makeCoords(KSpaceUtils);
    KSpaceUtils.assignViewOrdering(base, 220, 'croc', 105, 'ky', null, { macroWidth: 1 });

    const macro = makeCoords(KSpaceUtils);
    KSpaceUtils.assignViewOrdering(macro, 220, 'croc', 105, 'ky', null, { macroWidth: 10 });

    assert.ok(countShotChanges(base, macro) > 0, 'macro echoes can change shot membership');
    assertFiniteRms(macro.jumpMetrics);
});

test('large Chevron and CROC smoke cases have finite RMS jump', () => {
    const KSpaceUtils = loadKSpaceUtils();
    for (const ordering of ['chevron', 'croc']) {
        const coords = makeCoords(KSpaceUtils);
        KSpaceUtils.assignViewOrdering(coords, 220, ordering, 105, 'ky', null, { macroWidth: 10 });

        assert.equal(coords.length, 7520);
        assertUniqueShotEcho(coords);
        assertDirectMacroConstraints(coords, 10, 220);
        assertFiniteRms(coords.jumpMetrics);
    }
});

test('macro width RMS search exhaustively evaluates consecutive candidates', () => {
    const KSpaceUtils = loadKSpaceUtils();
    const result = KSpaceUtils.findBestMacroWidthByRms(
        () => makeCoords(KSpaceUtils),
        220,
        'croc',
        105,
        'ky',
        null,
        1,
        10
    );

    assert.equal(result.evaluatedWidths[0], 1);
    assert.equal(result.evaluatedWidths[result.evaluatedWidths.length - 1], 10);
    assert.ok(result.bestWidth >= 1 && result.bestWidth <= 10);
    assert.ok(Number.isFinite(result.bestRmsJump));
    assert.ok(result.worstWidth >= 1 && result.worstWidth <= 10);
    assert.ok(Number.isFinite(result.worstRmsJump));
    assert.equal(result.rmsByWidth.length, result.evaluatedWidths.length);
    for (let i = 1; i < result.evaluatedWidths.length; i++) {
        assert.equal(
            result.evaluatedWidths[i],
            result.evaluatedWidths[i - 1] + 1,
            'search evaluates consecutive widths'
        );
    }
});

test('macro width -1 auto mode selects the best candidate from 1 through 5', () => {
    const KSpaceUtils = loadKSpaceUtils();
    const coords = makeCoords(KSpaceUtils);
    KSpaceUtils.assignViewOrdering(coords, 220, 'croc', 105, 'ky', null, { macroWidth: -1 });

    assertUniqueShotEcho(coords);
    assertFiniteRms(coords.jumpMetrics);
    assert.ok(coords.macroSearch, 'auto macro search metadata is attached');
    assert.deepEqual(Array.from(coords.macroSearch.evaluatedWidths), [1, 2, 3, 4, 5]);
    assert.ok(coords.macroSearch.bestWidth >= 1 && coords.macroSearch.bestWidth <= 5);
    assert.ok(coords.macroSearch.worstWidth >= 1 && coords.macroSearch.worstWidth <= 5);
});
