// @ts-nocheck
import { defineConfig } from 'vite'
import preact from '@preact/preset-vite'
import { viteSingleFile } from "vite-plugin-singlefile"
import Sonda from 'sonda/vite'; 

import { writeFileSync, readFileSync } from 'node:fs';
import { resolve } from 'node:path';

import zlib from 'node:zlib';

async function zopfliCompress(input: string | Buffer): Promise<Buffer> {
  const args = ["zopfli", "--iterations=1500", "--gzip", "/dev/stdin", "-c"];

  try {
    // Spawn the zopfli process, passing the input via stdin
    const proc = Bun.spawn(args, {
      // @ts-ignore
      stdin: input,
      stdout: 'pipe',
      stderr: 'pipe',
    });

    await proc.exited;

    // Await the process completion and get the exit code
    const exitCode = proc.exitCode;

    // Check if the process exited with an error
    if (exitCode !== 0) {
      const stderr = await new Response(proc.stderr).text();
      throw new Error(`zopfli process exited with code ${exitCode}: ${stderr.trim()}`);
    }

    // Read the compressed data from stdout
    const compressedData = await new Response(proc.stdout).arrayBuffer();
    
    return Buffer.from(compressedData);
  } catch (error: any) {
    if (error.code === 'ENOENT') {
      throw new Error("The 'zopfli' command was not found. Please ensure it is installed and in your PATH.");
    }
    // Re-throw other errors
    throw error;
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
        
        // 2. Compress the HTML using gzip
        //const gzippedContent = Bun.gzipSync(htmlContent);
        const gzippedContent = await zopfliCompress(htmlContent);

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
        
        // 4. Write the C header file to the output directory
        writeFileSync(outputHeaderPath, headerFileContent.trim());
        writeFileSync(resolve(config.build.outDir, '../../Software/src/devboard/webserver/frontend.h'), headerFileContent.trim());

        // @ts-ignore
        this.info(`Successfully generated C array at: ${outputHeaderPath}, size: ${gzippedContent.length} bytes`);
        
      } catch (error) {
        // @ts-ignore
        this.error(`Failed to generate C array: ${error.message}`);
      }
    }
  };
}

import.meta.env.VITE_API_BASE = import.meta.env.NODE_ENV == "development" ? 'http://192.168.4.1' : '';
//import.meta.env.VITE_API_BASE = import.meta.env.NODE_ENV == "development" ? 'http://192.168.0.107:12345' : '';

// https://vite.dev/config/
export default defineConfig({
  build: {
    sourcemap: true,
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
