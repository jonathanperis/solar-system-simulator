// Retired URLs forward here: keep the query (?body=, ?lesson=) and fragment,
// which a meta refresh alone would drop. The meta refresh in the same page is
// the fallback when scripts are off.
(function () {
  var script = document.currentScript;
  var target = script && script.getAttribute('data-target');
  if (!target) return;
  var url = new URL(target, location.href);
  if (location.search && !url.search) url.search = location.search;
  if (location.hash && !url.hash) url.hash = location.hash;
  location.replace(url.href);
})();
