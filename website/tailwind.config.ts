import type { Config } from "tailwindcss";

const config: Config = {
  content: [
    "./app/**/*.{js,ts,jsx,tsx,mdx}",
    "./components/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  theme: {
    extend: {
      colors: {
        bg: "#131316",
        surface: "#1c1c21",
        surfaceAlt: "#26262d",
        border: "#33333c",
        accent: "#3b82f6",
        accentSoft: "#60a5fa",
        good: "#4ade80",
        warn: "#fbbf24",
        bad: "#f87171",
        muted: "#9ca3af",
      },
      fontFamily: {
        sans: ["Segoe UI", "system-ui", "-apple-system", "sans-serif"],
      },
      backgroundImage: {
        "grid-fade":
          "radial-gradient(ellipse 80% 50% at 50% -20%, rgba(59,130,246,0.15), transparent)",
      },
    },
  },
  plugins: [],
};

export default config;
