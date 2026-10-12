// Draws the help page from the texts in lang/xx.js (each one fills HELP.xx; the tabs follow the order index.html
// loads them in). They are scripts and not .json files so the page works opened from disk: a browser does not let
// a page read files from disk, but it does load its scripts.
var HELP = {};

function fill(selector, html) {
  document.querySelectorAll(selector).forEach(function (el) { el.innerHTML = html(el); });
}

function render(lang) {
  var t = HELP[lang];
  var age = String(new Date().getFullYear() - 2000);
  document.documentElement.lang = lang;
  fill("[data-t]", function (el) { return t[el.getAttribute("data-t")]; });
  fill("[data-list]", function (el) {
    return t[el.getAttribute("data-list")].map(function (s) { return "<li>" + s + "</li>"; }).join("");
  });
  fill("[data-fields]", function () {
    return t.fields.map(function (f) {
      return "<div><dt>" + f.label + "</dt><dd>" + f.value.replace("{age}", age) + "</dd></div>";
    }).join("");
  });
  fill("[data-vitals]", function () {
    return t.vitals.map(function (v) {
      return '<div class="vital"><span class="k">' + v.label + '</span><span class="v chunky">' + v.value +
        '</span><span class="ok">✓ ' + v.note + "</span></div>";
    }).join("");
  });
  fill(".opts", function () {
    return t.options.map(function (o) {
      var values = o.values.map(function (v) {
        return '<span class="val' + (v === o.default ? " def" : "") + '">' + v + "</span>";
      }).join("");
      return '<div class="opt"><div class="head"><span class="name">' + o.name + '</span><span class="vals">' +
        values + "</span></div><div>" + o.what + '</div><div class="why"><b>' + t.why + "</b> " + o.why + "</div></div>";
    }).join("");
  });
  document.querySelectorAll("nav.langs button").forEach(function (b) {
    b.setAttribute("aria-pressed", b.getAttribute("data-lang") === lang ? "true" : "false");
  });
  try { localStorage.setItem("runig-lang", lang); } catch (e) {}
}

// The language: the one in the address (#es), else the one picked here before, else the installer's, else the
// browser's, else English
function firstLang() {
  var hash = location.hash.replace("#", "");
  if (HELP[hash]) {
    return hash;
  }
  var saved = null;
  try { saved = localStorage.getItem("runig-lang"); } catch (e) {}
  if (HELP[saved]) {
    return saved;
  }
  if (HELP[INSTALLED_LANG]) {
    return INSTALLED_LANG;
  }
  var browser = (navigator.language || "en").slice(0, 2);
  return HELP[browser] ? browser : "en";
}

// Runs once every lang/xx.js has loaded (they come after this file)
document.addEventListener("DOMContentLoaded", function () {
  var nav = document.querySelector("nav.langs");
  nav.innerHTML = Object.keys(HELP).map(function (k) {
    return '<button type="button" data-lang="' + k + '">' + HELP[k].lang_name + "</button>";
  }).join("");
  nav.addEventListener("click", function (e) {
    var b = e.target.closest("button");
    if (b) {
      render(b.getAttribute("data-lang"));
    }
  });
  render(firstLang());
});
