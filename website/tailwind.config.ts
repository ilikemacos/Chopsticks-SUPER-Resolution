import type { Config } from "tailwindcss";

/**
 * Colour scale mirrors the reference build's CSS custom properties exactly;
 * every value resolves through the tokens defined in app/globals.css.
 */
const hsl = (v: string) => `hsl(var(${v}) / <alpha-value>)`;

const config: Config = {
  content: [
    "./app/**/*.{js,ts,jsx,tsx,mdx}",
    "./components/**/*.{js,ts,jsx,tsx,mdx}",
  ],
  theme: {
    extend: {
      colors: {
        bg: hsl("--background"),
        fg: hsl("--foreground"),
        border: hsl("--border"),
        card: hsl("--card"),
        cardBorder: hsl("--card-border"),
        primary: hsl("--primary"),
        primaryFg: hsl("--primary-foreground"),
        secondary: hsl("--secondary"),
        accent: hsl("--accent"),
        accentFg: hsl("--accent-foreground"),
        mutedBg: hsl("--muted"),
        muted: hsl("--muted-foreground"),
        faint: hsl("--muted-foreground"),
        input: hsl("--input"),
        ring: hsl("--ring"),
        destructive: hsl("--destructive"),
        good: hsl("--chart-3"),
        warn: hsl("--chart-4"),
        bad: hsl("--chart-5"),
      },
      fontFamily: {
        sans: ["var(--font-sans)", "Inter", "sans-serif"],
        display: ["var(--font-display)", "Space Grotesk", "sans-serif"],
        mono: ["var(--font-mono)", "JetBrains Mono", "monospace"],
      },
      borderRadius: {
        lg: "var(--radius)",
        xl: "calc(var(--radius) + 0.25rem)",
        "2xl": "1rem",
      },
      boxShadow: {
        glow: "var(--shadow-glow)",
        card: "var(--shadow-card)",
      },
      letterSpacing: {
        tight: "-0.025em",
        wider: "0.05em",
        widest: "0.1em",
      },
      animation: {
        float: "float 6s ease-in-out infinite",
        "pulse-slow": "pulse 4s ease-in-out infinite",
      },
    },
  },
  plugins: [],
};

export default config;
