// @ts-nocheck
import { defineConfig } from 'vite'
import preact from '@preact/preset-vite'
import { viteSingleFile } from "vite-plugin-singlefile"
import Sonda from 'sonda/vite'; 

import { writeFileSync, readFileSync, existsSync, mkdtempSync, rmSync } from 'node:fs';
import { resolve, join } from 'node:path';
import { execFileSync } from 'node:child_process';
import { tmpdir } from 'node:os';

import zlib from 'node:zlib';

// Build-time firmware version.
//
// Mirrors Software/src/devboard/utils/version.h exactly: reads the raw
// GIT_*/GITHUB_* defines that tools/identify_build.py wrote into
// version_autogen.h (a PlatformIO pre-script, so the header is always fresh
// when the frontend build runs) and assembles BUILD_VERSION with the same
// precedence. The result is baked into the bundle as __APP_VERSION__ and
// therefore into frontend.h, so the frontend and the backend firmware always
// report exactly the same version.
// Note: the config is bundled into node_modules/.vite-temp before it runs, so
// import.meta.url points at the temp copy, not this file. Anchor at cwd
// instead: build_frontend.py (and `bun run build` locally) always run from
// the frontend/ directory.
function findVersionAutogen(): string | null {
  const candidates = [
    resolve(process.cwd(), '../Software/src/devboard/utils/version_autogen.h'),
    resolve(process.cwd(), 'Software/src/devboard/utils/version_autogen.h'),
  ];
  for (const p of candidates) {
    if (existsSync(p)) return p;
  }
  return null;
}

function computeBuildVersion(): string {
  const autogenPath = findVersionAutogen();
  if (!autogenPath) return "unknown"; // no autogen header (e.g. standalone build)

  const defines: Record<string, string> = {};
  for (const line of readFileSync(autogenPath, 'utf8').split('\n')) {
    const m = line.match(/^#define\s+(\w+)\s+"(.*)"$/);
    if (m) defines[m[1]] = m[2];
  }

  if (defines.GIT_TAG) return defines.GIT_TAG;
  if (defines.GIT_ANCESTOR_TAG && defines.GIT_SHORT_SHA) {
    if (defines.GITHUB_PR && defines.GITHUB_PR_HEAD_SHORT_SHA) {
      return `${defines.GIT_ANCESTOR_TAG}dev-${defines.GITHUB_PR_HEAD_SHORT_SHA} (#${defines.GITHUB_PR})`;
    }
    if (defines.GIT_BRANCH) {
      return `${defines.GIT_ANCESTOR_TAG}dev-${defines.GIT_SHORT_SHA} (${defines.GIT_BRANCH})`;
    }
    return `${defines.GIT_ANCESTOR_TAG}dev-${defines.GIT_SHORT_SHA}`;
  }
  return "unknown";
}

const APP_VERSION = computeBuildVersion();
console.log(`[frontend] Building for firmware version: ${APP_VERSION}`);

async function zopfliCompress(input: string | Buffer): Promise<Buffer> {
  // zopfli cannot read from a pipe: /dev/stdin fails ("Invalid filename" /
  // "Files larger than 2GB are not supported") and it exits without producing
  // output, silently yielding an empty C array. It needs a seekable regular
  // file, so write the input to a temp file first.
  const dir = mkdtempSync(join(tmpdir(), 'bep-zopfli-'));
  const inputFile = join(dir, 'input.html');
  const args = ["--iterations=1500", "--gzip", inputFile, "-c"];

  try {
    writeFileSync(inputFile, input);
    const compressedData = execFileSync("zopfli", args, {
      maxBuffer: 64 * 1024 * 1024,
    });
    return Buffer.from(compressedData);
  } catch (error: any) {
    if (error.code === 'ENOENT') {
      throw new Error("The 'zopfli' command was not found. Please ensure it is installed and in your PATH.");
    }
    // Re-throw other errors
    throw error;
  } finally {
    rmSync(dir, { recursive: true, force: true });
  }
}



function gzipChunkOptimizer(options = {}) {
  const iterations = options.iterations ?? 1000;

  return {
    name: 'vite-plugin-gzip-chunk-optimizer',
    apply: 'build',

    async renderChunk(code, chunk) {
      const moduleIds = Object.keys(chunk.modules).filter(
        (id) => chunk.modules[id]?.code
      );

      // Skip chunks with single or no modules
      if (moduleIds.length <= 1) return null;

      // 1. Build local Dependency Graph (DAG) for modules inside this chunk
      const moduleSet = new Set(moduleIds);
      const depsMap = new Map(); // id -> Set(internal dependency IDs)

      for (const id of moduleIds) {
        const info = this.getModuleInfo(id);
        const internalDeps = (info?.importedIds || []).filter((depId) =>
          moduleSet.has(depId)
        );
        depsMap.set(id, new Set(internalDeps));
      }

      // 2. Constraint Checker: Ensures every dependency comes BEFORE its dependent
      function isValidOrder(order) {
        const pos = new Map();
        order.forEach((id, idx) => pos.set(id, idx));

        for (const [id, depSet] of depsMap.entries()) {
          const idPos = pos.get(id);
          for (const depId of depSet) {
            if (pos.get(depId) >= idPos) return false;
          }
        }
        return true;
      }

      // 3. Topological Sort Fallback (Kahn's Algorithm)
      function getTopologicalSort() {
        const inDegree = new Map(moduleIds.map((id) => [id, 0]));
        const graph = new Map(moduleIds.map((id) => [id, []]));

        for (const [id, depSet] of depsMap.entries()) {
          for (const dep of depSet) {
            graph.get(dep).push(id);
            inDegree.set(id, inDegree.get(id) + 1);
          }
        }

        const queue = moduleIds.filter((id) => inDegree.get(id) === 0);
        const sorted = [];

        while (queue.length > 0) {
          const u = queue.shift();
          sorted.push(u);
          for (const v of graph.get(u)) {
            inDegree.set(v, inDegree.get(v) - 1);
            if (inDegree.get(v) === 0) queue.push(v);
          }
        }

        return sorted.length === moduleIds.length ? sorted : moduleIds;
      }

      // Ensure starting order is valid
      const currentOrder = isValidOrder(moduleIds) ? [...moduleIds] : getTopologicalSort();

      // Helper to compute gzipped size of an order string
      function getGzipSize(order) {
        const combinedCode = order.map((id) => chunk.modules[id].code).join('\n');
        return zlib.gzipSync(Buffer.from(combinedCode), { level: 9 }).length;
      }

      let bestOrder = [...currentOrder];
      let bestSize = getGzipSize(bestOrder);
      const initialSize = bestSize;

      // 4. Stochastic Hill-Climbing
      for (let i = 0; i < iterations; i++) {
        const fromIdx = Math.floor(Math.random() * bestOrder.length);
        const toIdx = Math.floor(Math.random() * bestOrder.length);
        if (fromIdx === toIdx) continue;

        // Create candidate permutation by moving an item to a new index
        const candidate = [...bestOrder];
        const [movedItem] = candidate.splice(fromIdx, 1);
        candidate.splice(toIdx, 0, movedItem);

        // Verify DAG constraints first
        if (isValidOrder(candidate)) {
          const candidateSize = getGzipSize(candidate);
          // Standard hill-climbing acceptance criterion
          if (candidateSize < bestSize) {
            bestOrder = candidate;
            bestSize = candidateSize;
          }
        }
      }

      if (bestSize < initialSize) {
        const saved = initialSize - bestSize;
        const pct = ((saved / initialSize) * 100).toFixed(2);
        console.log(`[gzip-optimizer] ${chunk.fileName}: ${initialSize}B -> ${bestSize}B (-${saved}B / -${pct}%)`);
      }

      // Reconstruct optimized chunk code
      const optimizedCode = bestOrder.map((id) => chunk.modules[id].code).join('\n');

      return {
        code: optimizedCode,
        map: null
      };
    }
  };
}


function writeIfChanged(filePath: string, content: string) {
  // Only touch the file when the content actually changed, so SCons does not
  // see a stale header and rebuild webserver_new.cpp.o on every build.
  try {
    if (readFileSync(filePath, 'utf8') === content) return;
  } catch { /* file does not exist yet */ }
  writeFileSync(filePath, content);
}

/**
 * Vite/Rollup plugin to stochastically reorder modules within output
 * chunks to maximize gzip compression without breaking dependency order.
 */
// @ts-ignore
function zopfliChunkOptimizer(options = {}) {
  const iterations = options.iterations ?? 1000;

  return {
    name: 'vite-plugin-gzip-chunk-optimizer',
    apply: 'build',

    async renderChunk(code, chunk) {
      const moduleIds = Object.keys(chunk.modules).filter(
        (id) => chunk.modules[id]?.code
      );

      // Skip chunks with single or no modules
      if (moduleIds.length <= 1) return null;

      // 1. Build local Dependency Graph (DAG) for modules inside this chunk
      const moduleSet = new Set(moduleIds);
      const depsMap = new Map(); // id -> Set(internal dependency IDs)

      for (const id of moduleIds) {
        const info = this.getModuleInfo(id);
        const internalDeps = (info?.importedIds || []).filter((depId) =>
          moduleSet.has(depId)
        );
        depsMap.set(id, new Set(internalDeps));
      }

      // 2. Constraint Checker: Ensures every dependency comes BEFORE its dependent
      function isValidOrder(order) {
        const pos = new Map();
        order.forEach((id, idx) => pos.set(id, idx));

        for (const [id, depSet] of depsMap.entries()) {
          const idPos = pos.get(id);
          for (const depId of depSet) {
            if (pos.get(depId) >= idPos) return false;
          }
        }
        return true;
      }

      // 3. Topological Sort Fallback (Kahn's Algorithm)
      function getTopologicalSort() {
        const inDegree = new Map(moduleIds.map((id) => [id, 0]));
        const graph = new Map(moduleIds.map((id) => [id, []]));

        for (const [id, depSet] of depsMap.entries()) {
          for (const dep of depSet) {
            graph.get(dep).push(id);
            inDegree.set(id, inDegree.get(id) + 1);
          }
        }

        const queue = moduleIds.filter((id) => inDegree.get(id) === 0);
        const sorted = [];

        while (queue.length > 0) {
          const u = queue.shift();
          sorted.push(u);
          for (const v of graph.get(u)) {
            inDegree.set(v, inDegree.get(v) - 1);
            if (inDegree.get(v) === 0) queue.push(v);
          }
        }

        return sorted.length === moduleIds.length ? sorted : moduleIds;
      }

      // Ensure starting order is valid
      const currentOrder = isValidOrder(moduleIds) ? [...moduleIds] : getTopologicalSort();

      // Helper to compute gzipped size of an order string
      function getGzipSize(order) {
        const combinedCode = order.map((id) => chunk.modules[id].code).join('\n');
        return zlib.gzipSync(Buffer.from(combinedCode), { level: 9 }).length;
      }

      // Helper to compute Zopfli-compressed size of an order string
      async function getZopfliSize(order) {
        const combinedCode = order.map((id) => chunk.modules[id].code).join('\n');
        return (await zopfliCompress(Buffer.from(combinedCode))).length;
      }

      let bestOrder = [...currentOrder];
      //let bestSize = getGzipSize(bestOrder);
      let bestSize = await getZopfliSize(bestOrder);
      const initialSize = bestSize;

      // 4. Stochastic Hill-Climbing
      for (let i = 0; i < iterations; i++) {
        const fromIdx = Math.floor(Math.random() * bestOrder.length);
        const toIdx = Math.floor(Math.random() * bestOrder.length);
        if (fromIdx === toIdx) continue;

        // Create candidate permutation by moving an item to a new index
        const candidate = [...bestOrder];
        const [movedItem] = candidate.splice(fromIdx, 1);
        candidate.splice(toIdx, 0, movedItem);

        // Verify DAG constraints first
        if (isValidOrder(candidate)) {
          //const candidateSize = getGzipSize(candidate);
          const candidateSize = await getZopfliSize(candidate);
          // Standard hill-climbing acceptance criterion
          if (candidateSize < bestSize) {
            bestOrder = candidate;
            bestSize = candidateSize;
          }
        }
      }

      if (bestSize < initialSize) {
        const saved = initialSize - bestSize;
        const pct = ((saved / initialSize) * 100).toFixed(2);
        console.log(`[gzip-optimizer] ${chunk.fileName}: ${initialSize}B -> ${bestSize}B (-${saved}B / -${pct}%)`);
      }

      // Reconstruct optimized chunk code
      const optimizedCode = bestOrder.map((id) => chunk.modules[id].code).join('\n');

      return {
        code: optimizedCode,
        map: null
      };
    }
  };
}

// @ts-ignore
function htmlToCArray(options = {}) {
  const {
    outputFileName = 'html.h',
    arrayName = 'html_data'
  }: any = options;

  let config: any;

  return {
    name: 'vite-plugin-html-to-c-array',
    // This plugin only runs during the build process
    apply: 'build',

    // Store the resolved config
    configResolved(resolvedConfig: any) {
      config = resolvedConfig;
    },

    // This hook runs after the bundle is generated and written to disk
    async closeBundle() {
      // Resolve the path to the final index.html file
      const htmlFilePath = resolve(config.build.outDir, 'index.html');
      const outputHeaderPath = resolve(config.build.outDir, outputFileName);
      
      // @ts-ignore
      this.info(`Reading generated HTML from: ${htmlFilePath}`);

      try {
        // 1. Read the generated HTML file content
        const htmlContent = readFileSync(htmlFilePath);

        // 2. Compress the HTML. Prefer zopfli (smallest output); fall back to
        //    gzip so the build still works on machines without the zopfli
        //    binary installed (at the cost of a slightly larger embedded
        //    payload).
        let gzippedContent: Buffer;
        try {
          gzippedContent = await zopfliCompress(htmlContent);
        } catch (e: any) {
          if (String(e?.message || '').includes('zopfli')) {
            console.warn('[html-to-c-array] zopfli not found, using gzip (output will be larger)');
            gzippedContent = zlib.gzipSync(htmlContent, { level: 9 });
          } else {
            throw e;
          }
        }

        // Abort if compression silently produced nothing (e.g. a broken zopfli
        // binary exiting 0 without writing output): never embed an empty array.
        if (gzippedContent.length === 0) {
          this.error('Compression produced empty output - refusing to generate an empty C array');
        }

        // 3. Convert the gzipped buffer into a C array literal string
        // Each byte is formatted as a two-digit hexadecimal number (e.g., 0x0A)
        const bytesPerLine = 12;
        const hexBytes = Array.from(gzippedContent)
          .map(byte => `0x${byte.toString(16).padStart(2, '0')}`);

        const lines = [];
        // Loop in chunks of bytesPerLine
        for (let i = 0; i < hexBytes.length; i += bytesPerLine) {
          // Get a chunk of 12 bytes and join them
          const chunk = hexBytes.slice(i, i + bytesPerLine);
          lines.push(`  ${chunk.join(', ')}`); // Add indentation to the line
        }

        // Join all lines with a comma and a newline
        const cArrayLiteral = lines.join(',\n');

        // Create the full content for the C header file
        const headerFileContent = `
// This file is generated by vite-plugin-html-to-c-array
// Do not edit this file manually.

// Original HTML size: ${htmlContent.length} bytes
// Gzipped size: ${gzippedContent.length} bytes

// clang-format off
const unsigned char ${arrayName}[] = {
${cArrayLiteral}
};

const unsigned int ${arrayName}_len = ${gzippedContent.length};
`;
        
        // 4. Write the C header file to the output directory (only when the
        //    content changed, so consumers don't rebuild unnecessarily).
        writeIfChanged(outputHeaderPath, headerFileContent.trim());
        writeIfChanged(resolve(config.build.outDir, '../../Software/src/devboard/webserver/frontend.h'), headerFileContent.trim());

        // @ts-ignore
        this.info(`Successfully generated C array at: ${outputHeaderPath}, size: ${gzippedContent.length} bytes`);
        
      } catch (error) {
        // @ts-ignore
        this.error(`Failed to generate C array: ${error.message}`);
      }
    }
  };
}

process.env.VITE_API_BASE = process.env.NODE_ENV == "development" ? 'http://192.168.4.1' : '';
//process.env.VITE_API_BASE = process.env.NODE_ENV == "development" ? 'http://192.168.0.107:12345' : '';

// https://vite.dev/config/
export default defineConfig({
  define: {
    // Burnt into the bundle at build time; identical to the firmware's
    // BUILD_VERSION (see Software/src/devboard/utils/version.h).
    __APP_VERSION__: JSON.stringify(APP_VERSION),
  },
  build: {
    cssMinify: 'lightningcss',
    // Source maps are useless once the app is inlined into a single HTML file
    // and embedded in the device; disable them so the build doesn't emit a
    // large .js.map artifact or a sourceMappingURL comment in the shipped HTML.
    sourcemap: false,
    // Terser with multiple compression passes produces smaller output than
    // Vite's default esbuild minifier - worthwhile because the entire app is
    // inlined into one HTML file that gets embedded in device flash.
    minify: 'terser',
    terserOptions: {
      compress: {
        // Run the optimizer several times; each pass can unlock further wins.
        passes: 3,
        // Drop informational console.log() calls (e.g. CAN-log parse stats),
        // keep console.error/warn for real diagnostics.
        pure_funcs: ['console.log'],
      },
      mangle: true,
      // The app already ships modern syntax (optional chaining, nullish
      // coalescing, AbortSignal...); don't let terser hold back optimizations
      // for Safari 10.
      safari10: false,
      format: {
        comments: false,
        ecma: 2020,
      },
    },
  },
  //base: import.meta.env.VITE_DEMO_MODE === "true" ? '/be/demo/' : './',
  plugins: [
    preact(),
    //Sonda({gzip: true}),
    // @ts-ignore
    // gzipChunkOptimizer({
    //   iterations: 3000 // Adjust sampling iterations per chunk
    // }),
    // @ts-ignore
    {
      ...viteSingleFile(),
      apply: 'build'
    },
    // @ts-ignore
    htmlToCArray()
  ],
  // css: {
  //   transformer: 'lightningcss',
  //   lightningcss: {
  //     //targets: browserslistToTargets(browserslist('>= 0.25%'))
  //   }
  // },
  // build: {
  //   cssMinify: 'lightningcss'
  // }
})
