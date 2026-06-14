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

function assertDirectMacroConstraints(coords, macroWidth, etl) {
    for (const coord of coords) {
        assert.ok(Number.isInteger(coord.macroEcho) && coord.macroEcho >= 1, 'macro echo is assigned');
        const startEcho = (coord.macroEcho - 1) * macroWidth + 1;
        const endEcho = Math.min(etl, startEcho + macroWidth - 1);
        assert.ok(coord.echo >= startEcho && coord.echo <= endEcho, 'echo stays inside direct macro segment');
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

test('coarse-to-fine macro echoes assign unique slots inside macro segments', () => {
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

test('macro width RMS search starts at 1 and stops on first worsening step', () => {
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
    assert.ok(result.bestWidth >= 1 && result.bestWidth <= 10);
    assert.ok(Number.isFinite(result.bestRmsJump));
    for (let i = 1; i < result.evaluatedWidths.length; i++) {
        assert.equal(
            result.evaluatedWidths[i],
            result.evaluatedWidths[i - 1] + 1,
            'search evaluates consecutive widths'
        );
    }
});
