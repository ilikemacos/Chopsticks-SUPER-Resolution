import { site } from "@/lib/site";

const docs = [
  { title: "How upscaling works", href: `${site.docsUrl}/HOW_UPSCALING_WORKS.md` },
  { title: "How frame generation works", href: `${site.docsUrl}/HOW_FRAME_GENERATION_WORKS.md` },
  { title: "Why external injection is limited", href: `${site.docsUrl}/INJECTION_LIMITATIONS.md` },
  { title: "DX11 vs DX12", href: `${site.docsUrl}/DX11_VS_DX12.md` },
  { title: "FSR vs XeSS", href: `${site.docsUrl}/FSR_VS_XESS.md` },
  { title: "Game integration", href: `${site.docsUrl}/GAME_INTEGRATION.md` },
  { title: "Troubleshooting", href: `${site.docsUrl}/TROUBLESHOOTING.md` },
  { title: "Architecture", href: `${site.docsUrl}/ARCHITECTURE.md` },
];

export function Docs() {
  return (
    <section id="docs" className="mx-auto max-w-6xl px-4 py-20 sm:px-6">
      <h2 className="text-3xl font-semibold tracking-tight">Documentation</h2>
      <p className="mt-2 max-w-2xl text-muted">
        Technically accurate explanations, no marketing claims that cannot be
        demonstrated.
      </p>
      <div className="mt-8 grid gap-3 sm:grid-cols-2">
        {docs.map((d) => (
          <a
            key={d.title}
            href={d.href}
            className="flex items-center justify-between rounded-lg border border-border bg-surface px-5 py-4 transition-colors hover:border-accent/60"
          >
            <span>{d.title}</span>
            <span className="text-muted">→</span>
          </a>
        ))}
      </div>
    </section>
  );
}
