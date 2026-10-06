// Google Analytics bootstrap, loaded only by deployed main builds that set
// PUBLIC_GA_ID (V17). It lives in a static file instead of an inline script so
// the page Content-Security-Policy needs neither 'unsafe-inline' nor hashes.
(() => {
  const id = document.currentScript && document.currentScript.dataset.gaId;
  if (!id || !/^G-[A-Z0-9]+$/.test(id)) return;
  window.dataLayer = window.dataLayer || [];
  window.gtag = function gtag() { window.dataLayer.push(arguments); };
  window.gtag('js', new Date());
  window.gtag('config', id);
  const loader = document.createElement('script');
  loader.async = true;
  loader.src = `https://www.googletagmanager.com/gtag/js?id=${encodeURIComponent(id)}`;
  document.head.append(loader);
})();
