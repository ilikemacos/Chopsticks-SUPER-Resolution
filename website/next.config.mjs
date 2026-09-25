/** @type {import('next').NextConfig} */
const rawBase =
  "https://raw.githubusercontent.com/ilikemacos/Chopsticks-SUPER-Resolution/claude/great-babbage-lqfk8p/website/public";

// The native desktop app and the Windows installers are published as GitHub
// Release assets, built on Windows CI by the wpf-app.yml (.exe) and package.yml
// (.zip + .msi) workflows when a v* tag is pushed. Proxying them keeps the
// download links same-origin; GitHub answers with a redirect to its CDN, so this
// stays lightweight. These point at the current release tag.
const relBase = (tag) =>
  `https://github.com/ilikemacos/Chopsticks-SUPER-Resolution/releases/download/${tag}`;
// The .exe points at the latest release that actually has the asset (v0.2.0).
// The .msi and .zip point at v0.3.0, whose CI build attaches them; until that
// release is published the site hides those two buttons (site.installersReady).
const exeTag = "v0.2.0";
const installerTag = "v0.3.0";
const appExeUrl = `${relBase(exeTag)}/UniversalFrameFX.exe`;
const appMsiUrl = `${relBase(installerTag)}/UniversalFrameFX-x64.msi`;
const appZipUrl = `${relBase(installerTag)}/UniversalFrameFX-x64.zip`;

const nextConfig = {
  reactStrictMode: true,
  poweredByHeader: false,
  // Serve the download and installer from this origin by proxying the files
  // committed in the repo (public repo, so the raw URLs are anonymously
  // reachable). This keeps everything same-origin while the bytes come straight
  // from git, so a fixed install.ps1 goes live on push with no rebuild.
  async rewrites() {
    return [
      {
        source: "/UniversalFrameFX.exe",
        destination: appExeUrl,
      },
      {
        source: "/UniversalFrameFX-x64.msi",
        destination: appMsiUrl,
      },
      {
        source: "/UniversalFrameFX-x64.msi.sha256",
        destination: `${appMsiUrl}.sha256`,
      },
      {
        source: "/UniversalFrameFX-x64.zip",
        destination: appZipUrl,
      },
      {
        source: "/UniversalFrameFX-x64.zip.sha256",
        destination: `${appZipUrl}.sha256`,
      },
      {
        source: "/UniversalFrameFX-portable.zip",
        destination: `${rawBase}/UniversalFrameFX-portable.zip`,
      },
      {
        source: "/install.ps1",
        destination: `${rawBase}/install.ps1`,
      },
      {
        source: "/install.ps1.sha256",
        destination: `${rawBase}/install.ps1.sha256`,
      },
    ];
  },
};

export default nextConfig;
