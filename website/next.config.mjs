/** @type {import('next').NextConfig} */
const rawBase =
  "https://raw.githubusercontent.com/ilikemacos/Chopsticks-SUPER-Resolution/claude/great-babbage-lqfk8p/website/public";

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
