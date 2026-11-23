// The script of the course's web pages, written out by `rcpp site`.
// Progress lives in this browser only (localStorage); nothing is sent anywhere.
(function () {
  var KEY = "rcpp.done.v1";
  function load() {
    var s = {};
    try { (JSON.parse(localStorage.getItem(KEY) || "[]") || []).forEach(function (id) { s[id] = 1; }); } catch (e) {}
    // lessons passed with `rcpp verify` on this machine, when the pages were made locally
    (window.RCPP_PASSED || []).forEach(function (id) { s[id] = 1; });
    return s;
  }
  function save(s) {
    try { localStorage.setItem(KEY, JSON.stringify(Object.keys(s))); } catch (e) {}
  }
  var done = load();

  function refresh() {
    document.querySelectorAll("[data-lesson-check]").forEach(function (c) {
      var id = c.getAttribute("data-lesson-check");
      c.checked = !!done[id];
      var li = c.closest("li");
      if (li) li.classList.toggle("is-done", !!done[id]);
    });
    document.querySelectorAll("[data-progress]").forEach(function (el) {
      var ids = el.getAttribute("data-progress").split(" ").filter(Boolean);
      var n = 0;
      ids.forEach(function (id) { if (done[id]) n++; });
      var pct = ids.length ? Math.round((100 * n) / ids.length) : 0;
      var c = el.querySelector(".count");
      if (c) c.textContent = n + "/" + ids.length + " · " + pct + "%";
      var b = el.querySelector(".bar i");
      if (b) b.style.width = pct + "%";
      var d = el.querySelector(".done-n");
      if (d) d.textContent = n ? n + " done · " : "";
    });
  }

  document.addEventListener("change", function (e) {
    var c = e.target.closest("[data-lesson-check]");
    if (!c) return;
    var id = c.getAttribute("data-lesson-check");
    if (c.checked) done[id] = 1; else delete done[id];
    save(done);
    refresh();
  });

  // quizzes
  document.addEventListener("click", function (e) {
    var b = e.target.closest(".q button");
    if (!b) return;
    var q = b.closest(".q");
    if (q.classList.contains("answered")) return;
    var right = q.getAttribute("data-right");
    q.querySelectorAll("button").forEach(function (x) { if (x.getAttribute("data-i") === right) x.classList.add("right"); });
    if (b.getAttribute("data-i") !== right) b.classList.add("wrong");
    q.classList.add("answered");
  });

  // search boxes
  window.filterList = function (box) {
    var t = box.value.toLowerCase().trim();
    var scope = document.querySelector(box.getAttribute("data-scope") || "main");
    scope.querySelectorAll("[data-text]").forEach(function (li) {
      li.style.display = !t || li.getAttribute("data-text").indexOf(t) >= 0 ? "" : "none";
    });
  };

  // the self-check
  window.selfCheck = function () {
    var items = document.querySelectorAll("[data-can]");
    var start = null;
    for (var i = 0; i < items.length; i++) {
      if (!items[i].checked) { start = items[i].getAttribute("data-can"); break; }
    }
    var out = document.getElementById("verdict");
    if (!out) return;
    if (start === null) start = out.getAttribute("data-last");
    var link = document.querySelector('[data-phase-link="' + start + '"]');
    out.innerHTML = "";
    var p = document.createElement("p");
    p.className = "serif";
    p.style.fontSize = "26px";
    p.style.margin = "0 0 8px";
    p.textContent = "Start at phase " + start + (link ? ", " + link.getAttribute("data-title") : "") + ".";
    out.appendChild(p);
    if (link) {
      var a = document.createElement("a");
      a.className = "btn";
      a.href = link.getAttribute("href");
      a.textContent = "Open phase " + start + " →";
      out.appendChild(a);
    }
    out.style.display = "block";
  };

  // light and dark
  var root = document.documentElement;
  try { var th = localStorage.getItem("rcpp.theme"); if (th) root.setAttribute("data-theme", th); } catch (e) {}
  window.toggleTheme = function () {
    var dark = window.matchMedia && window.matchMedia("(prefers-color-scheme: light)").matches ? false : true;
    var cur = root.getAttribute("data-theme") || (dark ? "dark" : "light");
    var next = cur === "dark" ? "light" : "dark";
    root.setAttribute("data-theme", next);
    try { localStorage.setItem("rcpp.theme", next); } catch (e) {}
  };

  refresh();
})();
