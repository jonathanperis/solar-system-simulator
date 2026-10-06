import { atlasPositionCss } from '../../lib/atlasLayout';

// Prerendered at build time into /styles/atlas-positions.css. Body positions
// derive from implementedBodies, so the stylesheet cannot drift from the atlas.
export function GET(): Response {
  return new Response(atlasPositionCss(), { headers: { 'Content-Type': 'text/css; charset=utf-8' } });
}
