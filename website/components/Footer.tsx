import { site } from "@/lib/site";

export function Footer() {
  return (
    <footer className="border-t border-border">
      <div className="mx-auto flex max-w-6xl flex-col items-center justify-between gap-4 px-4 py-10 text-sm text-muted sm:flex-row sm:px-6">
        <p>
          Universal FrameFX — MIT licensed. Not affiliated with AMD, Intel or
          NVIDIA.
        </p>
        <div className="flex gap-6">
          <a href={site.githubUrl} className="hover:text-fg">
            GitHub
          </a>
          <a href={site.docsUrl} className="hover:text-fg">
            Docs
          </a>
          <a href="#download" className="hover:text-fg">
            Download
          </a>
        </div>
      </div>
    </footer>
  );
}
