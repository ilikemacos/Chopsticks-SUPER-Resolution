import type { Config } from "tailwindcss";

const config: Config = {
  content: [
    "./app/**/*.{js,ts,jsx,tsx,mdx}",
    "./components/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  theme: {
    extend: {
      colors: {
        bg: "#0a0a0b",
        surface: "#17171c",
        surfaceAlt: "#1f1f27",
        border: "#26262e",
        accent: "#5865f2",
        accentSoft: "#8b93f7",
        good: "#00c758",
        warn: "#f5b84b",
        bad: "#f0616d",
        muted: "#8a8a95",
        faint: "#5c5c66",
      },
      fontFamily: {
        sans: ["var(--font-sans)", "Inter", "system-ui", "sans-serif"],
        display: ["var(--font-display)", "Space Grotesk", "sans-serif"],
        mono: ["var(--font-mono)", "JetBrains Mono", "monospace"],
      },
      boxShadow: {
        glow: "0 0 60px -12px rgba(88,101,242,0.45)",
      },
      backgroundImage: {
        "grid-fade":
          "radial-gradient(ellipse 80% 50% at 50% -20%, rgba(88,101,242,0.18), transparent)",
      },
    },
  },
  plugins: [],
};

export default config;
