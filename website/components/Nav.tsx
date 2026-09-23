import Link from "next/link";
import { site } from "@/lib/site";

const links = [
  { href: "#features", label: "Features" },
  { href: "#compatibility", label: "Compatibility" },
  { href: "#download", label: "Download" },
  { href: "#docs", label: "Docs" },
  { href: "#faq", label: "FAQ" },
];

export function Nav() {
  return (
    <header className="sticky top-0 z-50 border-b border-border/60 bg-bg/80 backdrop-blur">
      <nav className="mx-auto flex max-w-6xl items-center justify-between px-4 py-3 sm:px-6">
        <Link href="/" className="flex items-center gap-2 font-semibold">
          <span className="grid h-7 w-7 place-items-center rounded-md bg-accent text-sm font-bold text-white">
            FX
          </span>
          <span>Universal FrameFX</span>
        </Link>
        <div className="hidden items-center gap-6 md:flex">
          {links.map((l) => (
            <a
              key={l.href}
              href={l.href}
              className="text-sm text-muted transition-colors hover:text-white"
            >
              {l.label}
            </a>
          ))}
        </div>
        <a
          href={site.githubUrl}
          className="rounded-md border border-border px-3 py-1.5 text-sm text-white transition-colors hover:border-accent hover:text-accentSoft"
        >
          View on GitHub
        </a>
      </nav>
    </header>
  );
}
