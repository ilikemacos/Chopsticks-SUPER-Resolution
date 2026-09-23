/** @type {import('next').NextConfig} */
const rawBase =
  "https://raw.githubusercontent.com/ilikemacos/Chopsticks-SUPER-Resolution/claude/great-babbage-lqfk8p/website/public";

const nextConfig = {
  reactStrictMode: true,
  poweredByHeader: false,
  // Serve the portable zip from this origin by proxying the file committed in
  // the repo (public repo, so the raw URL is anonymously reachable). This keeps
  // the download same-origin without shipping the binary through the build.
  async rewrites() {
    return [
      {
        source: "/UniversalFrameFX-portable.zip",
        destination: `${rawBase}/UniversalFrameFX-portable.zip`,
      },
    ];
  },
};

export default nextConfig;
