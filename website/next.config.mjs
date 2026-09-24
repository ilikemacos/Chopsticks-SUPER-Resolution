/** @type {import('next').NextConfig} */
const rawBase =
  "https://raw.githubusercontent.com/ilikemacos/Chopsticks-SUPER-Resolution/claude/great-babbage-lqfk8p/website/public";

// The native desktop app is published as a GitHub Release asset. Proxying it
// keeps the download link same-origin; GitHub answers with a redirect to its
// CDN, so this stays lightweight.
const appExeUrl =
  "https://github.com/ilikemacos/Chopsticks-SUPER-Resolution/releases/download/v0.2.0/UniversalFrameFX.exe";

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
