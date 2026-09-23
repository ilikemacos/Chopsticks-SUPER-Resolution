// Central place for repository / download URLs.
const repoUrl = "https://github.com/ilikemacos/Chopsticks-SUPER-Resolution";
const siteBase = "https://universal-framefx.vercel.app";

export const site = {
  owner: "ilikemacos",
  repo: "Chopsticks-SUPER-Resolution",
  githubUrl: repoUrl,
  releasesUrl: `${repoUrl}/releases`,
  // The native .NET 8 / WPF desktop app — a real double-click .exe. Served
  // same-origin (proxied to the GitHub Release asset).
  appExeUrl: "/UniversalFrameFX.exe",
  appVersion: "v0.1.0",
  // The runnable PowerShell edition, served directly from this site — no
  // compiler, no GitHub redirect. Relative paths download from this origin.
  portableZipUrl: "/UniversalFrameFX-portable.zip",
  installScriptUrl: `${siteBase}/install.ps1`,
  docsUrl: `${repoUrl}/tree/claude/great-babbage-lqfk8p/docs`,
};
