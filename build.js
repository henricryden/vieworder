/**
 * Build script: obfuscate JS files and copy assets to dist/
 * Run: npm run build
 */

const fs = require('fs');
const path = require('path');
const JavaScriptObfuscator = require('javascript-obfuscator');

const dashboardDir = path.join(__dirname, 'dashboard');
const distDir = path.join(__dirname, 'dist');

// Ensure dist directory exists
if (!fs.existsSync(distDir)) {
    fs.mkdirSync(distDir, { recursive: true });
}

/**
 * Obfuscate a JS file using javascript-obfuscator
 */
function obfuscateJS(inputPath, outputPath) {
    try {
        const code = fs.readFileSync(inputPath, 'utf8');
        
        // Determine which variables to preserve based on file
        let reservedNames = [];
        if (inputPath.includes('kspace-utils.js')) {
            reservedNames = ['KSpaceUtils'];
        } else if (inputPath.includes('d3-plots.js')) {
            reservedNames = ['D3Plots'];
        } else if (inputPath.includes('app.js')) {
            reservedNames = ['App'];
        }

        const obfuscated = JavaScriptObfuscator.obfuscate(code, {
            compact: true,
            controlFlowFlattening: true,
            controlFlowFlatteningThreshold: 0.75,
            deadCodeInjection: true,
            deadCodeInjectionThreshold: 0.4,
            debugProtection: false,
            disableConsoleOutput: false,
            identifierNamesGenerator: 'hexadecimal',
            log: false,
            renameGlobals: false,
            rotateStringArray: true,
            selfDefending: false,
            stringArray: true,
            stringArrayThreshold: 0.75,
            unicodeEscapeSequence: false,
            reservedNames: reservedNames
        });

        fs.writeFileSync(outputPath, obfuscated.getObfuscatedCode());
        console.log(`✓ Obfuscated: ${inputPath} → ${outputPath}`);
    } catch (err) {
        console.error(`Error obfuscating ${inputPath}:`, err);
        process.exit(1);
    }
}

/**
 * Copy CSS file (no obfuscation needed)
 */
function copyCSS(inputPath, outputPath) {
    try {
        const css = fs.readFileSync(inputPath, 'utf8');
        // Optional: minify CSS (simple removal of whitespace)
        const minified = css
            .replace(/\/\*[\s\S]*?\*\//g, '')  // Remove comments
            .replace(/\s+/g, ' ')              // Collapse whitespace
            .replace(/\s*([{}:;,])\s*/g, '$1') // Remove space around punctuation
            .trim();
        fs.writeFileSync(outputPath, minified);
        console.log(`✓ Copied: ${inputPath} → ${outputPath}`);
    } catch (err) {
        console.error(`Error copying CSS:`, err);
        process.exit(1);
    }
}

/**
 * Create obfuscated single-file HTML (optional, for offline use)
 * Disabled for now - separate files in dist/ are safer and work better with GitHub Pages
 */
async function createSingleFileHTML() {
    console.log('⊘ Skipped single-file HTML (use separate dist/index.html + .js files instead)');
}

/**
 * Main build process
 */
async function build() {
    console.log('Building...\n');
    
    // Obfuscate JS files
    const jsFiles = ['kspace-utils.js', 'd3-plots.js', 'app.js'];
    for (const file of jsFiles) {
        obfuscateJS(
            path.join(dashboardDir, file),
            path.join(distDir, file)
        );
    }
    
    // Copy and minify CSS
    copyCSS(
        path.join(dashboardDir, 'style.css'),
        path.join(distDir, 'style.css')
    );
    
    // Copy HTML (no changes needed)
    fs.copyFileSync(
        path.join(dashboardDir, 'index.html'),
        path.join(distDir, 'index.html')
    );
    console.log(`✓ Copied: ${path.join(dashboardDir, 'index.html')} → ${path.join(distDir, 'index.html')}`);
    
    // Create single-file version
    await createSingleFileHTML();
    
    console.log('\n✓ Build complete! Files ready in ./dist/');
    console.log('\nFor GitHub Pages:');
    console.log('  1. Create a gh-pages branch or use /docs folder');
    console.log('  2. Copy dist/ contents to gh-pages or /docs');
    console.log('  3. Enable GitHub Pages in repository settings');
}

build().catch(err => {
    console.error('Build failed:', err);
    process.exit(1);
});
