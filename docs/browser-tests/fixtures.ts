import { test as base, expect } from '@playwright/test';

/**
 * Hosts the deployed analytics bootstrap may contact (see src/lib/csp.ts).
 * Canonical main builds embed PUBLIC_GA_ID, so their browser tests load the
 * gtag loader too. Every test stubs those requests: CI must never send real
 * page views, and the outcome must not depend on Google being reachable.
 */
export const analyticsUrl = /^https:\/\/([a-z0-9-]+\.)*(googletagmanager\.com|google-analytics\.com|analytics\.google\.com)\//;

export const test = base.extend<{ analyticsRequests: string[] }>({
  analyticsRequests: [async ({ context }, use) => {
    const requests: string[] = [];
    // The request only reaches this handler if the page CSP allowed it, so
    // recording it also proves the policy admits the analytics loader.
    await context.route(analyticsUrl, route => {
      requests.push(route.request().url());
      return route.fulfill({ status: 204, body: '' });
    });
    await use(requests);
  }, { auto: true }]
});

export { expect };
