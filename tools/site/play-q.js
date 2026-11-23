// Train a robot by trial: Q-learning on the pit grid of phase 39 (lessons 39-01 to 39-04), in the browser.
// The same task: '#' wall, 'S' start, 'G' goal (+10, ends), 'P' pit (-10, ends), every step -1, moves slip sideways.
(function () {
  var ROWS = ["#########", "#.......#", "#.#####.#", "#.#####.#", "#S.....G#", "#PPPPPPP#", "#########"];
  var W = ROWS[0].length, H = ROWS.length, cells = ROWS.join("");
  var cv = document.getElementById("grid"), ctx = cv.getContext("2d");
  var el = function (id) { return document.getElementById(id); };
  var Q, episodes, pits, goals, seed, lastPath = [];

  function rnd() { seed = (seed * 1664525 + 1013904223) >>> 0; return seed / 4294967296; }
  function terminal(s) { var c = cells[s]; return c === "G" || c === "P"; }
  function reward(s) { var c = cells[s]; return c === "G" ? 10 : c === "P" ? -10 : 0; }
  var DX = [0, 1, 0, -1], DY = [-1, 0, 1, 0];
  function moved(s, m) { var x = s % W + DX[m], y = Math.floor(s / W) + DY[m]; var n = y * W + x; return cells[n] === "#" ? s : n; }
  function step(s, m, slip) { var u = rnd(); if (u < slip) m = (m + 1) % 4; else if (u < 2 * slip) m = (m + 3) % 4; return moved(s, m); }
  function best(s) { var b = 0; for (var m = 1; m < 4; m++) if (Q[s * 4 + m] > Q[s * 4 + b]) b = m; return b; }

  function reset() {
    Q = new Float64Array(cells.length * 4).fill(+el("q0").value);
    for (var s = 0; s < cells.length; s++) if (terminal(s)) for (var m = 0; m < 4; m++) Q[s * 4 + m] = 0;
    episodes = 0; pits = 0; goals = 0; seed = 12345; lastPath = []; draw();
  }

  function train(n) {
    var slip = +el("slip").value, gamma = +el("gamma").value, eps = +el("eps").value, alpha = 0.1, start = cells.indexOf("S");
    for (var e = 0; e < n; e++) {
      var s = start, path = [s];
      for (var t = 0; t < 200; t++) {
        var m = rnd() < eps ? Math.floor(rnd() * 4) : best(s);
        var nx = step(s, m, slip), end = terminal(nx);
        var target = -1 + reward(nx) + (end ? 0 : gamma * Q[nx * 4 + best(nx)]);
        Q[s * 4 + m] += alpha * (target - Q[s * 4 + m]);
        s = nx; path.push(s);
        if (end) break;
      }
      if (cells[s] === "P") pits++; else if (cells[s] === "G") goals++;
      episodes++; lastPath = path;
    }
    draw();
  }

  function size() {
    var r = window.devicePixelRatio || 1, w = cv.clientWidth, h = Math.round(w * H / W);
    cv.width = w * r; cv.height = h * r; cv.style.height = h + "px"; ctx.setTransform(r, 0, 0, r, 0, 0);
    draw();
  }

  function draw() {
    var cs = getComputedStyle(document.documentElement), w = cv.clientWidth, c = w / W;
    var fg = cs.getPropertyValue("--fg").trim(), dim = cs.getPropertyValue("--dim").trim(), line = cs.getPropertyValue("--line").trim(), acc = cs.getPropertyValue("--accent").trim(), bad = cs.getPropertyValue("--bad").trim(), ok = cs.getPropertyValue("--ok").trim(), bg2 = cs.getPropertyValue("--bg2").trim();
    ctx.clearRect(0, 0, w, cv.clientHeight);
    var vmax = 0;
    for (var s = 0; s < cells.length; s++) if (cells[s] === "." || cells[s] === "S") vmax = Math.max(vmax, Math.abs(Q[s * 4 + best(s)]));
    for (var y = 0; y < H; y++) for (var x = 0; x < W; x++) {
      var i = y * W + x, ch = cells[i], px = x * c, py = y * c;
      if (ch === "#") { ctx.fillStyle = bg2; ctx.fillRect(px, py, c, c); continue; }
      if (ch === "P") { ctx.fillStyle = bad; ctx.globalAlpha = 0.35; ctx.fillRect(px + 1, py + 1, c - 2, c - 2); ctx.globalAlpha = 1; continue; }
      if (ch === "G") { ctx.fillStyle = ok; ctx.globalAlpha = 0.35; ctx.fillRect(px + 1, py + 1, c - 2, c - 2); ctx.globalAlpha = 1; ctx.fillStyle = fg; ctx.font = (c * 0.32) + "px ui-monospace, monospace"; ctx.fillText("+10", px + c * 0.2, py + c * 0.6); continue; }
      var v = Q[i * 4 + best(i)];
      ctx.strokeStyle = line; ctx.strokeRect(px + 0.5, py + 0.5, c - 1, c - 1);
      ctx.fillStyle = acc; ctx.globalAlpha = vmax > 0 ? Math.min(0.5, Math.max(0, (v + vmax) / (2 * vmax)) * 0.5) : 0; ctx.fillRect(px + 1, py + 1, c - 2, c - 2); ctx.globalAlpha = 1;
      if (episodes > 0) {
        var m = best(i), cx = px + c / 2, cy = py + c / 2, L = c * 0.28;
        ctx.strokeStyle = fg; ctx.lineWidth = 2; ctx.beginPath();
        ctx.moveTo(cx - DX[m] * L, cy - DY[m] * L); ctx.lineTo(cx + DX[m] * L, cy + DY[m] * L);
        ctx.lineTo(cx + DX[m] * L - (DX[m] - DY[m]) * L * 0.45, cy + DY[m] * L - (DY[m] + DX[m]) * L * 0.45);
        ctx.moveTo(cx + DX[m] * L, cy + DY[m] * L); ctx.lineTo(cx + DX[m] * L - (DX[m] + DY[m]) * L * 0.45, cy + DY[m] * L - (DY[m] - DX[m]) * L * 0.45);
        ctx.stroke();
      }
      if (ch === "S") { ctx.fillStyle = dim; ctx.font = (c * 0.26) + "px ui-monospace, monospace"; ctx.fillText("start", px + 3, py + c - 5); }
    }
    if (lastPath.length > 1) {
      ctx.strokeStyle = acc; ctx.lineWidth = 2; ctx.globalAlpha = 0.8; ctx.beginPath();
      lastPath.forEach(function (s, k) { var x = (s % W + 0.5) * c, y = (Math.floor(s / W) + 0.5) * c; if (k === 0) ctx.moveTo(x, y); else ctx.lineTo(x, y); });
      ctx.stroke(); ctx.globalAlpha = 1;
    }
    el("v-slip").textContent = (+el("slip").value).toFixed(2); el("v-gamma").textContent = (+el("gamma").value).toFixed(2); el("v-eps").textContent = (+el("eps").value).toFixed(2); el("v-q0").textContent = (+el("q0").value).toFixed(0);
    el("r-ep").textContent = episodes; el("r-goal").textContent = goals; el("r-pit").textContent = pits;
    var start = cells.indexOf("S"), route = episodes === 0 ? "—" : best(start) === 0 ? "the long way round" : best(start) === 1 ? "the corridor by the pits" : "undecided";
    el("r-route").textContent = route;
  }

  el("t1").addEventListener("click", function () { train(1); });
  el("t100").addEventListener("click", function () { train(100); });
  el("t1000").addEventListener("click", function () { train(1000); });
  el("reset").addEventListener("click", reset);
  ["slip", "gamma", "eps"].forEach(function (id) { el(id).addEventListener("input", draw); });
  el("q0").addEventListener("input", reset);
  window.addEventListener("resize", size);
  reset();
  size();
})();
