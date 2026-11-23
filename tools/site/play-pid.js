// Tune a PID loop: a motor holding an angle, as in phase 14. The plant is J theta'' = u - b theta' + push,
// the controller u = Kp e + Ki integral(e) + Kd de/dt, limited to the motor's largest torque. The motor has a little
// dry friction FC as well: at rest it does not move until the torque passes FC, which is what leaves a P loop short.
(function () {
  var cv = document.getElementById("pid"), ctx = cv.getContext("2d");
  var el = function (id) { return document.getElementById(id); };
  var J = 0.02, B = 0.05, FC = 0.08, UMAX = 2.0, DT = 0.002, SECS = 4.0, TARGET = 1.0;
  var pushAt = -1;

  function simulate(kp, ki, kd, push) {
    var th = 0, w = 0, integ = 0, prevE = TARGET, out = [], n = Math.round(SECS / DT), peak = 0, settle = -1, sat = 0;
    for (var i = 0; i <= n; i++) {
      var t = i * DT, e = TARGET - th;
      integ += e * DT;
      var d = (e - prevE) / DT; prevE = e;
      var u = kp * e + ki * integ + kd * d;
      if (u > UMAX) { u = UMAX; sat++; } else if (u < -UMAX) { u = -UMAX; sat++; }
      var f = push && t >= 2.0 && t < 2.1 ? -1.5 : 0;
      var drive = u + f, fr = 0;
      if (Math.abs(w) > 1e-4) fr = w > 0 ? FC : -FC; else fr = Math.abs(drive) <= FC ? drive : (drive > 0 ? FC : -FC);
      w += (drive - fr - B * w) / J * DT;
      if (Math.abs(w) <= 1e-4 && Math.abs(drive) <= FC) w = 0;
      th += w * DT;
      if (!isFinite(th) || Math.abs(th) > 50) { out.push(NaN); return { out: out, unstable: true }; }
      out.push(th);
      if (t < 2.0 || !push) peak = Math.max(peak, th);
    }
    // the last time it was outside 2 percent of the target, before any push
    var lastOut = 0, upto = push ? Math.round(2.0 / DT) : n;
    for (var k = 0; k < upto; k++) if (Math.abs(out[k] - TARGET) > 0.02 * TARGET) lastOut = k;
    settle = lastOut < upto - 1 ? (lastOut + 1) * DT : -1;
    return { out: out, unstable: false, overshoot: Math.max(0, (peak - TARGET) / TARGET * 100), settle: settle, sat: sat / n * 100, final: out[upto] };
  }

  function size() {
    var r = window.devicePixelRatio || 1, w = cv.clientWidth, h = Math.round(w * 0.55);
    cv.width = w * r; cv.height = h * r; cv.style.height = h + "px"; ctx.setTransform(r, 0, 0, r, 0, 0);
    draw();
  }

  function draw() {
    var kp = +el("kp").value, ki = +el("ki").value, kd = +el("kd").value, push = el("push").checked;
    el("v-kp").textContent = kp.toFixed(2); el("v-ki").textContent = ki.toFixed(2); el("v-kd").textContent = kd.toFixed(3);
    var s = simulate(kp, ki, kd, push);
    var w = cv.clientWidth, h = cv.clientHeight, cs = getComputedStyle(document.documentElement);
    var fg = cs.getPropertyValue("--fg").trim(), dim = cs.getPropertyValue("--dim").trim(), line = cs.getPropertyValue("--line").trim(), acc = cs.getPropertyValue("--accent").trim();
    ctx.clearRect(0, 0, w, h);
    var pad = 30, ymax = 1.8, X = function (t) { return pad + (w - 2 * pad) * t / SECS; }, Y = function (v) { return h - pad - (h - 2 * pad) * v / ymax; };
    ctx.strokeStyle = line; ctx.lineWidth = 1;
    for (var t = 0; t <= SECS; t += 1) { ctx.beginPath(); ctx.moveTo(X(t), pad / 2); ctx.lineTo(X(t), h - pad); ctx.stroke(); ctx.fillStyle = dim; ctx.font = "11px ui-monospace, monospace"; ctx.fillText(t + " s", X(t) - 8, h - 10); }
    ctx.fillStyle = cs.getPropertyValue("--accent-bg").trim();
    ctx.fillRect(pad, Y(1.02), w - 2 * pad, Y(0.98) - Y(1.02));
    ctx.strokeStyle = dim; ctx.setLineDash([5, 4]); ctx.beginPath(); ctx.moveTo(pad, Y(TARGET)); ctx.lineTo(w - pad, Y(TARGET)); ctx.stroke(); ctx.setLineDash([]);
    if (push) { ctx.fillStyle = acc; ctx.fillText("push", X(2.0) + 4, pad); ctx.strokeStyle = acc; ctx.beginPath(); ctx.moveTo(X(2.0), pad); ctx.lineTo(X(2.0), h - pad); ctx.stroke(); }
    ctx.strokeStyle = s.unstable ? cs.getPropertyValue("--bad").trim() : fg; ctx.lineWidth = 2; ctx.beginPath();
    for (var i = 0; i < s.out.length; i += 2) {
      var v = Math.max(-0.2, Math.min(ymax + 0.2, s.out[i])); if (!isFinite(v)) break;
      if (i === 0) ctx.moveTo(X(i * DT), Y(v)); else ctx.lineTo(X(i * DT), Y(v));
    }
    ctx.stroke();
    el("r-os").textContent = s.unstable ? "—" : s.overshoot.toFixed(1) + " %";
    el("r-ts").textContent = s.unstable ? "—" : s.settle < 0 ? "never" : s.settle.toFixed(2) + " s";
    el("r-sat").textContent = s.unstable ? "—" : s.sat.toFixed(0) + " %";
    el("r-ss").textContent = s.unstable ? "—" : (TARGET - s.final).toFixed(4) + " rad";
    var msg = s.unstable ? "Unstable: the loop rang until it ran away. Too much gain for this plant."
      : ki === 0 && Math.abs(TARGET - s.final) > 0.002 ? "No integral term, and the motor's dry friction holds it short of the target: a steady error that only Ki removes."
      : s.overshoot > 20 ? "A lot of overshoot: more Kd damps it, less Kp calms it."
      : s.sat > 30 ? "The motor spends much of the run at its torque limit; the gains ask for more than it has."
      : "A tidy step. Now tick the push and see whether it recovers.";
    el("r-msg").textContent = msg;
  }

  ["kp", "ki", "kd", "push"].forEach(function (id) { el(id).addEventListener("input", draw); });
  el("preset").addEventListener("click", function () { el("kp").value = 2.0; el("ki").value = 1.0; el("kd").value = 0.12; draw(); });
  window.addEventListener("resize", size);
  size();
})();
