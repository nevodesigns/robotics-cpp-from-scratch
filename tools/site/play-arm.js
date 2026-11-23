// Drag a robot arm: the two-link planar arm of lesson 32-01, in the browser.
// The same closed form as rc::arm::scara_ik: cos q2 = (x^2 + y^2 - l1^2 - l2^2) / (2 l1 l2), two elbows.
(function () {
  var cv = document.getElementById("arm"), ctx = cv.getContext("2d");
  var L1 = 0.225, L2 = 0.225;            // m, as the lesson's SCARA
  var Q1LIM = 2.35, Q2LIM = 2.6;          // rad, the joint limits
  var target = { x: 0.25, y: 0.18 }, elbow = 1, dragging = false;
  var el = function (id) { return document.getElementById(id); };

  function size() {
    var r = window.devicePixelRatio || 1, w = cv.clientWidth, h = Math.round(w * 0.8);
    cv.width = w * r; cv.height = h * r; cv.style.height = h + "px"; ctx.setTransform(r, 0, 0, r, 0, 0);
    draw();
  }
  function view() {
    var w = cv.clientWidth, h = cv.clientHeight, s = Math.min(w, h) / (2.25 * (L1 + L2));
    return { ox: w / 2, oy: h / 2, s: s };
  }
  function toScreen(p) { var v = view(); return { x: v.ox + p.x * v.s, y: v.oy - p.y * v.s }; }
  function toWorld(px, py) { var v = view(); return { x: (px - v.ox) / v.s, y: (v.oy - py) / v.s }; }

  function ik(x, y, sign) {
    var c2 = (x * x + y * y - L1 * L1 - L2 * L2) / (2 * L1 * L2);
    if (c2 > 1 || c2 < -1) return null;
    var q2 = sign * Math.acos(c2);
    var q1 = Math.atan2(y, x) - Math.atan2(L2 * Math.sin(q2), L1 + L2 * Math.cos(q2));
    return { q1: q1, q2: q2 };
  }
  function fk(q1, q2) {
    return { ex: L1 * Math.cos(q1), ey: L1 * Math.sin(q1), x: L1 * Math.cos(q1) + L2 * Math.cos(q1 + q2), y: L1 * Math.sin(q1) + L2 * Math.sin(q1 + q2) };
  }

  function draw() {
    var w = cv.clientWidth, h = cv.clientHeight, cs = getComputedStyle(document.documentElement);
    var fg = cs.getPropertyValue("--fg").trim(), dim = cs.getPropertyValue("--dim").trim(), line = cs.getPropertyValue("--line").trim(), acc = cs.getPropertyValue("--accent").trim(), bad = cs.getPropertyValue("--bad").trim();
    ctx.clearRect(0, 0, w, h);
    var o = toScreen({ x: 0, y: 0 }), v = view();
    // seen from above: the circle the hand can reach, and the base's axes
    ctx.strokeStyle = line; ctx.lineWidth = 1; ctx.setLineDash([4, 4]);
    ctx.beginPath(); ctx.arc(o.x, o.y, (L1 + L2) * v.s, 0, 2 * Math.PI); ctx.stroke(); ctx.setLineDash([]);
    ctx.beginPath(); ctx.moveTo(o.x - (L1 + L2) * v.s * 1.05, o.y); ctx.lineTo(o.x + (L1 + L2) * v.s * 1.05, o.y);
    ctx.moveTo(o.x, o.y - (L1 + L2) * v.s * 1.05); ctx.lineTo(o.x, o.y + (L1 + L2) * v.s * 1.05); ctx.stroke();

    var sol = ik(target.x, target.y, elbow), within = sol && Math.abs(sol.q1) <= Q1LIM && Math.abs(sol.q2) <= Q2LIM;
    var q = sol || { q1: Math.atan2(target.y, target.x), q2: 0 };
    var p = fk(q.q1, q.q2), e = toScreen({ x: p.ex, y: p.ey }), hnd = toScreen({ x: p.x, y: p.y });
    var det = L1 * L2 * Math.sin(q.q2), near = Math.abs(Math.sin(q.q2)) < 0.15;

    ctx.lineCap = "round";
    ctx.strokeStyle = sol ? (within ? fg : bad) : dim; ctx.lineWidth = 9;
    ctx.beginPath(); ctx.moveTo(o.x, o.y); ctx.lineTo(e.x, e.y); ctx.lineTo(hnd.x, hnd.y); ctx.stroke();
    [o, e].forEach(function (j) { ctx.fillStyle = acc; ctx.beginPath(); ctx.arc(j.x, j.y, 6, 0, 7); ctx.fill(); });
    ctx.fillStyle = fg; ctx.beginPath(); ctx.arc(hnd.x, hnd.y, 4, 0, 7); ctx.fill();
    // the Jacobian's columns at the hand: how a small turn of each joint moves it
    var j1 = { x: -(L1 * Math.sin(q.q1) + L2 * Math.sin(q.q1 + q.q2)), y: L1 * Math.cos(q.q1) + L2 * Math.cos(q.q1 + q.q2) };
    var j2 = { x: -L2 * Math.sin(q.q1 + q.q2), y: L2 * Math.cos(q.q1 + q.q2) };
    [[j1, acc], [j2, dim]].forEach(function (c) {
      ctx.strokeStyle = c[1]; ctx.lineWidth = 2; ctx.beginPath(); ctx.moveTo(hnd.x, hnd.y);
      ctx.lineTo(hnd.x + c[0].x * v.s * 0.6, hnd.y - c[0].y * v.s * 0.6); ctx.stroke();
    });
    // the target
    var t = toScreen(target);
    ctx.strokeStyle = acc; ctx.lineWidth = 2; ctx.beginPath(); ctx.arc(t.x, t.y, 11, 0, 7); ctx.stroke();
    ctx.beginPath(); ctx.moveTo(t.x - 16, t.y); ctx.lineTo(t.x + 16, t.y); ctx.moveTo(t.x, t.y - 16); ctx.lineTo(t.x, t.y + 16); ctx.stroke();

    var deg = function (a) { return (a * 180 / Math.PI).toFixed(1) + "°"; };
    el("r-x").textContent = target.x.toFixed(3) + " m";
    el("r-y").textContent = target.y.toFixed(3) + " m";
    el("r-q1").textContent = sol ? deg(q.q1) : "—";
    el("r-q2").textContent = sol ? deg(q.q2) : "—";
    el("r-det").textContent = sol ? det.toFixed(4) : "—";
    var msg = !sol ? "Out of reach: no angles put the hand there. The solver says so instead of stretching toward it (E-ARM-0011)."
      : !within ? "Reachable, but this elbow breaks a joint limit. Try the other elbow (E-ARM-0103)."
      : near ? "Near a singularity: the determinant l1 l2 sin q2 is close to zero, so the hand can hardly move along the arm (E-ARM-0104)."
      : "Reachable with this elbow. The two lines at the hand are the Jacobian's columns.";
    el("r-msg").textContent = msg;
    el("r-msg").className = sol && within && !near ? "" : "warn";
  }

  function pointer(ev) {
    var r = cv.getBoundingClientRect();
    var p = ev.touches ? ev.touches[0] : ev;
    return toWorld(p.clientX - r.left, p.clientY - r.top);
  }
  cv.addEventListener("pointerdown", function (ev) { dragging = true; cv.setPointerCapture(ev.pointerId); target = pointer(ev); draw(); });
  cv.addEventListener("pointermove", function (ev) { if (dragging) { target = pointer(ev); draw(); } });
  cv.addEventListener("pointerup", function () { dragging = false; });
  el("elbow").addEventListener("click", function () { elbow = -elbow; el("elbow").textContent = elbow > 0 ? "Elbow: right" : "Elbow: left"; draw(); });
  el("straight").addEventListener("click", function () { target = { x: 0.449, y: 0.0 }; draw(); });
  window.addEventListener("resize", size);
  size();
})();
