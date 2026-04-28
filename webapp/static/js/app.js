// ===========================================================================
// Dijkstra MPI visualizer — main client-side script
// ===========================================================================

"use strict";

// ---------------------------------------------------------------------------
// State
// ---------------------------------------------------------------------------

const state = {
    filename: null,
    n: 0,
    directed: false,
    edges: [],
    layout: null,
    needsClientLayout: false,

    // Solve result (from /api/compute) — 2D: indexed by source.
    allDist: null,        // [[...], [...], ...] or null
    allParent: null,      // [[...], [...], ...] or null
    selectedSource: 0,    // which source the user has picked
    runMeta: null,        // { num_processes, io_seconds, compute_seconds, elapsed_seconds, runner }

    // UI state
    selectedTarget: null,
    showLabels: false,
    showWeights: false,
    showTree: true,
};

// ---------------------------------------------------------------------------
// Sigma + graphology setup
// ---------------------------------------------------------------------------

const Graph = graphology.Graph;

// Resolve the forceAtlas2 layout function across UMD naming conventions.
const forceAtlas2 =
    window.graphologyLayoutForceAtlas2 ||
    (window.graphologyLibrary && window.graphologyLibrary.layoutForceAtlas2) ||
    null;

let sigmaGraph = null;
let renderer = null;

function initSigma() {
    const container = document.getElementById("sigma-container");
    if (renderer) {
        renderer.kill();
        renderer = null;
    }
    sigmaGraph = new Graph({ type: state.directed ? "directed" : "undirected", multi: false });
    renderer = new Sigma(sigmaGraph, container, {
        renderEdgeLabels: state.showWeights,
        renderLabels: state.showLabels,
        labelColor: { color: "#cfd3da" },
        labelSize: 12,
        labelFont: "JetBrains Mono",
        edgeLabelColor: { color: "#8a9099" },
        edgeLabelSize: 10,
        edgeLabelFont: "JetBrains Mono",
        defaultEdgeColor: "#2c3540",
        defaultNodeColor: "#cfd3da",
        labelDensity: 0.7,
        labelGridCellSize: 60,
        minCameraRatio: 0.05,
        maxCameraRatio: 10,
    });

    renderer.on("clickNode", ({ node, event }) => {
        const v = parseInt(node, 10);
        if (event && event.original && (event.original.altKey || event.original.metaKey)) {
            if (state.allDist) {
                state.selectedSource = v;
                state.selectedTarget = null;
                updateSourceSelector();
            }
        } else {
            state.selectedTarget = v;
        }
        applyHighlights();
        renderSelectedInfo();
    });

    renderer.on("clickStage", () => {
        state.selectedTarget = null;
        applyHighlights();
        renderSelectedInfo();
    });
}

// ---------------------------------------------------------------------------
// Build / rebuild graph data in sigma
// ---------------------------------------------------------------------------

function buildGraph() {
    sigmaGraph.clear();

    // Place nodes
    for (let i = 0; i < state.n; i++) {
        const pos = state.layout
            ? state.layout[String(i)]
            : { x: Math.cos((2 * Math.PI * i) / state.n), y: Math.sin((2 * Math.PI * i) / state.n) };
        sigmaGraph.addNode(String(i), {
            x: pos.x,
            y: pos.y,
            size: nodeSize(state.n),
            color: "#cfd3da",
            label: String(i),
        });
    }

    // Place edges
    for (const [u, v, w] of state.edges) {
        const key = `${u}__${v}`;
        sigmaGraph.addEdgeWithKey(key, String(u), String(v), {
            size: edgeSize(state.n),
            color: "#2c3540",
            label: String(w),
            weight: w,
        });
    }

    // If layout was not provided by server, run forceatlas2 client-side.
    if (state.needsClientLayout && forceAtlas2) {
        const settings = forceAtlas2.inferSettings(sigmaGraph);
        const positions = forceAtlas2(sigmaGraph, {
            iterations: state.n > 5000 ? 80 : 200,
            settings,
        });
        sigmaGraph.forEachNode((node) => {
            sigmaGraph.mergeNodeAttributes(node, positions[node]);
        });
    }
}

function nodeSize(n) {
    if (n <= 50) return 10;
    if (n <= 500) return 6;
    if (n <= 5000) return 3;
    return 2;
}

function edgeSize(n) {
    if (n <= 50) return 1.5;
    if (n <= 500) return 0.7;
    return 0.3;
}

// ---------------------------------------------------------------------------
// Highlighting — colours nodes and edges based on selectedTarget + parent[]
// ---------------------------------------------------------------------------

function currentDist() {
    return state.allDist ? state.allDist[state.selectedSource] : null;
}

function currentParent() {
    return state.allParent ? state.allParent[state.selectedSource] : null;
}

function applyHighlights() {
    if (!sigmaGraph) return;

    const dist = currentDist();
    const parent = currentParent();
    const source = state.allDist ? state.selectedSource : null;
    const { selectedTarget, showTree } = state;

    const onPath = new Set();
    const pathEdges = new Set();

    if (parent && selectedTarget !== null && selectedTarget !== undefined) {
        const path = reconstructPath(parent, source, selectedTarget);
        if (path) {
            for (const v of path) onPath.add(v);
            for (let i = 0; i + 1 < path.length; i++) {
                pathEdges.add(edgeKey(path[i], path[i + 1]));
            }
        }
    }

    const treeEdges = new Set();
    if (parent && showTree) {
        for (let v = 0; v < state.n; v++) {
            const p = parent[v];
            if (p !== -1 && p !== undefined && p !== null) {
                treeEdges.add(edgeKey(p, v));
            }
        }
    }

    // Nodes
    sigmaGraph.forEachNode((nodeId) => {
        const v = parseInt(nodeId, 10);
        let color = "#cfd3da";
        let size = nodeSize(state.n);

        if (dist && dist[v] === -1 && v !== source) {
            color = "#4a525e";
        }

        // selected target wins over path nodes
        if (v === selectedTarget) {
            color = "#00d9c0";
            size = nodeSize(state.n) * 1.8;
        } else if (v === source) {
            // source is always gold (also when it sits on the path)
            color = "#f0b429";
            size = nodeSize(state.n) * 1.6;
        } else if (onPath.has(v)) {
            color = "#00d9c0";
            size = nodeSize(state.n) * 1.25;
        }

        sigmaGraph.setNodeAttribute(nodeId, "color", color);
        sigmaGraph.setNodeAttribute(nodeId, "size", size);
    });

    // Edges
    sigmaGraph.forEachEdge((edgeId) => {
        let color = "#2c3540";
        let size = edgeSize(state.n);

        // pathEdges should win over treeEdges (visually on top), so check tree first
        if (treeEdges.has(edgeId)) {
            color = "#4a90e2";
            size = edgeSize(state.n) * 1.4;
        }
        if (pathEdges.has(edgeId)) {
            color = "#00d9c0";
            size = edgeSize(state.n) * 2.2;
        }

        sigmaGraph.setEdgeAttribute(edgeId, "color", color);
        sigmaGraph.setEdgeAttribute(edgeId, "size", size);
    });
}

function edgeKey(a, b) {
    // Match the key we used when adding edges. For undirected graphs we stored
    // edges as min__max from the adjacency-matrix loop (i < j); for directed
    // we stored each direction separately. Try both directions.
    if (sigmaGraph.hasEdge(`${a}__${b}`)) return `${a}__${b}`;
    if (sigmaGraph.hasEdge(`${b}__${a}`)) return `${b}__${a}`;
    return `${a}__${b}`;
}

function reconstructPath(parent, source, target) {
    if (target === source) return [source];
    if (parent[target] === -1) return null;

    const path = [];
    let v = target;
    let safety = parent.length + 2;
    while (v !== -1 && safety-- > 0) {
        path.push(v);
        if (v === source) break;
        v = parent[v];
    }
    if (v !== source) return null; // bad chain
    return path.reverse();
}

// ---------------------------------------------------------------------------
// API helpers
// ---------------------------------------------------------------------------

async function api(path, body) {
    const res = await fetch(path, {
        method: body ? "POST" : "GET",
        headers: { "Content-Type": "application/json" },
        body: body ? JSON.stringify(body) : undefined,
    });
    if (!res.ok) {
        const err = await res.json().catch(() => ({ error: res.statusText }));
        throw new Error(err.error || res.statusText);
    }
    return res.json();
}

// ---------------------------------------------------------------------------
// UI: graph list, load, compute
// ---------------------------------------------------------------------------

async function refreshGraphList() {
    const select = document.getElementById("graph-select");
    try {
        const list = await api("/api/graphs");
        select.innerHTML = '<option value="">— select —</option>';
        for (const g of list) {
            const opt = document.createElement("option");
            opt.value = g.name;
            opt.textContent = `${g.name}  (${g.size_kb} kB)`;
            select.appendChild(opt);
        }
    } catch (e) {
        toast(`Failed to list graphs: ${e.message}`, true);
    }
}

async function loadSelectedGraph() {
    const filename = document.getElementById("graph-select").value;
    if (!filename) {
        toast("Pick a graph first", true);
        return;
    }
    setBusy(true, "Loading graph…");
    try {
        const data = await api("/api/graph", { filename });
        state.filename = filename;
        state.n = data.n;
        state.edges = data.edges;
        state.directed = data.directed;
        state.layout = data.layout;
        state.needsClientLayout = data.needs_client_layout;
        state.allDist = null;
        state.allParent = null;
        state.selectedSource = 0;
        state.runMeta = null;
        state.selectedTarget = null;

        initSigma();
        buildGraph();
        applyHighlights();
        renderSelectedInfo();
        renderStats({ n: state.n });
        document.getElementById("canvas-overlay").classList.add("hidden");
        document.getElementById("btn-compute").disabled = false;
        document.getElementById("graph-meta").innerHTML =
            `<span class="ok">✓</span> ${state.n} nodes, ${state.edges.length} edges, ${state.directed ? "directed" : "undirected"}` +
            (state.needsClientLayout ? `<br>layout: client-side (forceAtlas2)` : `<br>layout: server-side`);
        toast(`Loaded ${filename}`);
    } catch (e) {
        toast(`Load failed: ${e.message}`, true);
    } finally {
        setBusy(false);
    }
}

async function runCompute() {
    if (!state.filename) return;
    const runner = document.getElementById("runner-select").value;
    const np = parseInt(document.getElementById("np-input").value, 10) || 1;

    setBusy(true, `Computing with ${runner}, np=${np}…`);
    try {
        const result = await api("/api/compute", {
            filename: state.filename,
            runner,
            np,
        });
        state.allDist = result.dist;
        state.allParent = result.parent;
        state.selectedSource = 0;
        state.selectedTarget = null;
        state.runMeta = {
            num_processes: result.num_processes,
            io_seconds: result.io_seconds,
            compute_seconds: result.compute_seconds,
            elapsed_seconds: result.elapsed_seconds,
            runner: result.runner,
        };
        updateSourceSelector();
        applyHighlights();
        renderSelectedInfo();
        renderStats(result);
        toast(`Done in ${formatSeconds(result.elapsed_seconds)}`);
    } catch (e) {
        toast(`Compute failed: ${e.message}`, true);
    } finally {
        setBusy(false);
    }
}

function updateSourceSelector() {
    const select = document.getElementById("source-select");
    if (!select) return;
    select.innerHTML = "";
    if (!state.allDist) {
        select.disabled = true;
        return;
    }
    select.disabled = false;
    for (let s = 0; s < state.n; s++) {
        const opt = document.createElement("option");
        opt.value = String(s);
        opt.textContent = `node ${s}`;
        select.appendChild(opt);
    }
    select.value = String(state.selectedSource);
}

function onSourceChange(e) {
    const s = parseInt(e.target.value, 10);
    if (Number.isFinite(s)) {
        state.selectedSource = s;
        state.selectedTarget = null;
        applyHighlights();
        renderSelectedInfo();
    }
}

// ---------------------------------------------------------------------------
// Right-side panels
// ---------------------------------------------------------------------------

function renderSelectedInfo() {
    const el = document.getElementById("selected-info");
    const pathEl = document.getElementById("path-info");

    const dist = currentDist();
    const parent = currentParent();
    const source = state.allDist ? state.selectedSource : null;

    if (state.selectedTarget === null || state.selectedTarget === undefined) {
        el.innerHTML = `<p class="dim">Click a node to inspect its shortest path.</p>`;
        pathEl.innerHTML = `<p class="dim">—</p>`;
        return;
    }

    const v = state.selectedTarget;
    const d = dist ? dist[v] : null;

    let html = `<p><strong>Node ${v}</strong></p>`;
    if (d === null) {
        html += `<p class="dim">No solver result yet — press <em>Compute shortest paths</em>.</p>`;
    } else if (d === -1) {
        html += `<p style="color: var(--warn);">Unreachable from source ${source}.</p>`;
    } else {
        html += `<p>Distance from source ${source}: <strong>${d}</strong></p>`;
        const parentV = parent[v];
        html += `<p class="dim">Parent in tree: ${parentV === -1 ? "—" : parentV}</p>`;
    }
    el.innerHTML = html;

    if (parent && d !== null && d !== -1) {
        const path = reconstructPath(parent, source, v);
        if (path) {
            const chain = path
                .map((x) => `<span>${x}</span>`)
                .join('<span class="arrow">→</span>');
            pathEl.innerHTML = `<div class="path-chain">${chain}</div>` +
                `<p class="dim">${path.length - 1} hops, total weight ${d}</p>`;
        } else {
            pathEl.innerHTML = `<p class="dim">No path.</p>`;
        }
    } else {
        pathEl.innerHTML = `<p class="dim">—</p>`;
    }
}

function renderStats(result) {
    const set = (id, v) => (document.getElementById(id).textContent = v);
    set("stat-n", result.n ?? state.n ?? "—");
    set("stat-source", state.allDist ? state.selectedSource : "—");
    set("stat-np", result.num_processes ?? "—");
    set("stat-runner", result.runner ?? "—");
    set("stat-io", result.io_seconds !== undefined ? formatSeconds(result.io_seconds) : "—");
    set("stat-compute", result.compute_seconds !== undefined ? formatSeconds(result.compute_seconds) : "—");
    set("stat-total", result.elapsed_seconds !== undefined ? formatSeconds(result.elapsed_seconds) : "—");
}

// ---------------------------------------------------------------------------
// Misc UI helpers
// ---------------------------------------------------------------------------

function formatSeconds(s) {
    if (s === undefined || s === null) return "—";
    if (s < 1e-3) return `${(s * 1e6).toFixed(1)} µs`;
    if (s < 1) return `${(s * 1e3).toFixed(2)} ms`;
    return `${s.toFixed(3)} s`;
}

function toast(msg, isError = false) {
    const el = document.getElementById("canvas-toast");
    el.textContent = msg;
    el.classList.toggle("error", isError);
    el.hidden = false;
    clearTimeout(toast._t);
    toast._t = setTimeout(() => (el.hidden = true), 3500);
}

function setBusy(busy, msg) {
    const compute = document.getElementById("btn-compute");
    const load = document.getElementById("btn-load");
    compute.disabled = busy || !state.filename;
    load.disabled = busy;
    if (busy && msg) toast(msg);
}

// ---------------------------------------------------------------------------
// Wire up
// ---------------------------------------------------------------------------

document.addEventListener("DOMContentLoaded", () => {
    initSigma();
    refreshGraphList();

    document.getElementById("btn-load").addEventListener("click", loadSelectedGraph);
    document.getElementById("btn-compute").addEventListener("click", runCompute);

    document.getElementById("source-select").addEventListener("change", onSourceChange);

    document.getElementById("toggle-tree").addEventListener("change", (e) => {
        state.showTree = e.target.checked;
        applyHighlights();
    });

    document.getElementById("toggle-labels").addEventListener("change", (e) => {
        state.showLabels = e.target.checked;
        if (renderer) {
            renderer.setSetting("renderLabels", state.showLabels);
            renderer.refresh();
        }
    });

    document.getElementById("toggle-weights").addEventListener("change", (e) => {
        state.showWeights = e.target.checked;
        if (renderer) {
            renderer.setSetting("renderEdgeLabels", state.showWeights);
            renderer.refresh();
        }
    });

    document.getElementById("btn-relayout").addEventListener("click", () => {
        if (!sigmaGraph || !forceAtlas2) {
            toast("Layout library not available", true);
            return;
        }
        const settings = forceAtlas2.inferSettings(sigmaGraph);
        const positions = forceAtlas2(sigmaGraph, { iterations: 200, settings });
        sigmaGraph.forEachNode((node) => sigmaGraph.mergeNodeAttributes(node, positions[node]));
        renderer.refresh();
        toast("Layout updated");
    });
});