import Link from "next/link";
import { site } from "@/lib/site";

const links = [
  { href: "/console", label: "Console" },
  { href: "/#features", label: "Features" },
  { href: "/#compatibility", label: "Compatibility" },
  { href: "/#download", label: "Download" },
  { href: "/#docs", label: "Docs" },
  { href: "/#faq", label: "FAQ" },
];

export function Nav() {
  return (
    <header className="sticky top-0 z-50 border-b border-border bg-bg/70 backdrop-blur-xl">
      <nav className="mx-auto flex max-w-6xl items-center justify-between px-4 py-3.5 sm:px-6">
        <Link href="/" className="flex items-center gap-2.5 font-display font-semibold">
          <span className="grid h-8 w-8 place-items-center rounded-lg bg-primary text-sm font-bold text-primaryFg shadow-glow">
            FX
          </span>
          <span className="tracking-tight">Universal FrameFX</span>
        </Link>
        <div className="hidden items-center gap-7 md:flex">
          {links.map((l) => (
            <a
              key={l.href}
              href={l.href}
              className="text-sm text-muted transition-colors hover:text-fg"
            >
              {l.label}
            </a>
          ))}
        </div>
        <a
          href={site.githubUrl}
          className="rounded-lg border border-border px-3.5 py-1.5 text-sm text-fg transition-colors hover:border-primary/30 hover:text-primary"
        >
          GitHub
        </a>
      </nav>
    </header>
  );
}
