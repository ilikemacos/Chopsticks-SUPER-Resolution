# Universal FrameFX — Website

Marketing and documentation site for Universal FrameFX. Next.js 15 (App Router) +
TypeScript + Tailwind CSS, deployable to Vercel with zero configuration.

## Develop

```bash
npm install
npm run dev      # http://localhost:3000
```

## Build

```bash
npm run build
npm run start    # serve the production build
npm run lint
```

## Deploy to Vercel

1. Import the repository in Vercel.
2. Set the **Root Directory** to `website`.
3. Framework preset: **Next.js** (auto-detected). No env vars required.

## Editing content

- Compatibility table data: `lib/compat.ts` — populate cells only with vendor-
  verified information.
- Repository / release URLs: `lib/site.ts` — replace `OWNER` with the GitHub org.
- Sections live in `components/` and are composed in `app/page.tsx`.
- SEO / OpenGraph metadata: `app/layout.tsx`.

The site makes no universal-compatibility claims; the copy mirrors the honest
capability matrix in `docs/ARCHITECTURE.md`.
