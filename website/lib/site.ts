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
  // The Windows installer (WiX MSI) and the C++ x64 zip, both built on Windows
  // CI and proxied to the GitHub Release assets. The MSI installs per-machine;
  // the zip is the portable build carrying the app plus the CSR command-line
  // tools (ufx-upscale, ufx-live).
  appMsiUrl: "/UniversalFrameFX-x64.msi",
  appZipUrl: "/UniversalFrameFX-x64.zip",
  // The downloadable native app today is v0.2.0 (its release carries the .exe).
  appVersion: "v0.2.0",
  // The .msi and portable .zip ship with the v0.3.0 release; until that CI build
  // is published, hide those two buttons rather than link to assets that 404.
  installersReady: false,
  // The runnable PowerShell edition, served directly from this site — no
  // compiler, no GitHub redirect. Relative paths download from this origin.
  portableZipUrl: "/UniversalFrameFX-portable.zip",
  installScriptUrl: `${siteBase}/install.ps1`,
  docsUrl: `${repoUrl}/tree/claude/great-babbage-lqfk8p/docs`,
};
