"use client";

import type { ReactNode } from "react";

export function Card({
  children,
  className = "",
}: {
  children: ReactNode;
  className?: string;
}) {
  return <div className={`glass rounded-2xl p-6 ${className}`}>{children}</div>;
}

export function SectionLabel({ children }: { children: ReactNode }) {
  return <p className="section-label">{children}</p>;
}

export function PanelHeading({
  kicker,
  title,
  blurb,
}: {
  kicker: string;
  title: string;
  blurb?: string;
}) {
  return (
    <div className="mb-8">
      <SectionLabel>{kicker}</SectionLabel>
      <h2 className="mt-2 font-display text-3xl font-bold tracking-tight text-gradient">
        {title}
      </h2>
      {blurb ? <p className="mt-3 max-w-2xl text-sm leading-relaxed text-muted">{blurb}</p> : null}
    </div>
  );
}

type Tone = "good" | "warn" | "bad" | "accent" | "muted";

const toneClasses: Record<Tone, string> = {
  good: "border-good/40 bg-good/10 text-good",
  warn: "border-warn/40 bg-warn/10 text-warn",
  bad: "border-bad/40 bg-bad/10 text-bad",
  accent: "border-accent/40 bg-accent/10 text-accentSoft",
  muted: "border-white/10 bg-white/5 text-muted",
};

export function Pill({ tone = "muted", children }: { tone?: Tone; children: ReactNode }) {
  return (
    <span
      className={`inline-flex shrink-0 items-center rounded-full border px-2.5 py-0.5 font-mono text-[10px] uppercase tracking-wider ${toneClasses[tone]}`}
    >
      {children}
    </span>
  );
}

export function Field({
  label,
  hint,
  children,
}: {
  label: string;
  hint?: string;
  children: ReactNode;
}) {
  return (
    <label className="block">
      <span className="mb-1.5 block font-mono text-[10px] uppercase tracking-widest text-faint">
        {label}
      </span>
      {children}
      {hint ? <span className="mt-1 block text-xs text-faint">{hint}</span> : null}
    </label>
  );
}

const controlClass =
  "w-full rounded-lg border border-border bg-black/40 px-3 py-2 text-sm text-white outline-none transition-colors focus:border-accent";

export function TextInput(props: React.InputHTMLAttributes<HTMLInputElement>) {
  return <input {...props} className={`${controlClass} ${props.className ?? ""}`} />;
}

export function Select(props: React.SelectHTMLAttributes<HTMLSelectElement>) {
  return <select {...props} className={`${controlClass} ${props.className ?? ""}`} />;
}

export function Slider({
  value,
  onChange,
  min = 0,
  max = 1,
  step = 0.05,
}: {
  value: number;
  onChange: (v: number) => void;
  min?: number;
  max?: number;
  step?: number;
}) {
  return (
    <div className="flex items-center gap-3">
      <input
        type="range"
        min={min}
        max={max}
        step={step}
        value={value}
        onChange={(e) => onChange(Number(e.target.value))}
        className="h-1.5 w-full cursor-pointer appearance-none rounded-full bg-white/10 accent-accent"
      />
      <span className="w-12 shrink-0 text-right font-mono text-xs text-muted">
        {value.toFixed(2)}
      </span>
    </div>
  );
}

export function Btn({
  children,
  onClick,
  variant = "ghost",
  disabled,
  type = "button",
}: {
  children: ReactNode;
  onClick?: () => void;
  variant?: "primary" | "ghost" | "danger";
  disabled?: boolean;
  type?: "button" | "submit";
}) {
  const base =
    "inline-flex items-center justify-center gap-2 rounded-lg px-4 py-2 text-sm font-medium transition-colors disabled:cursor-not-allowed disabled:opacity-45";
  const styles =
    variant === "primary"
      ? "bg-accent text-white hover:bg-accentSoft shadow-glow"
      : variant === "danger"
        ? "border border-bad/40 text-bad hover:bg-bad/10"
        : "border border-border text-white hover:border-accent";
  return (
    <button type={type} onClick={onClick} disabled={disabled} className={`${base} ${styles}`}>
      {children}
    </button>
  );
}

export function KeyVal({
  k,
  v,
  tone,
}: {
  k: string;
  v: ReactNode;
  tone?: "muted" | "bad";
}) {
  return (
    <div className="flex items-start justify-between gap-4 border-b border-white/5 py-2 last:border-0">
      <span className="shrink-0 text-xs text-muted">{k}</span>
      <span
        className={`text-right text-xs ${tone === "bad" ? "text-bad" : tone === "muted" ? "text-faint" : "text-white"}`}
      >
        {v}
      </span>
    </div>
  );
}

export function Empty({
  title,
  body,
  action,
}: {
  title: string;
  body: string;
  action?: ReactNode;
}) {
  return (
    <div className="rounded-2xl border border-dashed border-white/10 bg-white/[0.02] p-10 text-center">
      <p className="font-display text-base font-semibold text-white">{title}</p>
      <p className="mx-auto mt-2 max-w-lg text-sm leading-relaxed text-muted">{body}</p>
      {action ? <div className="mt-5 flex justify-center gap-3">{action}</div> : null}
    </div>
  );
}

/** Reads a user-selected JSON file. Returns null when parsing fails. */
export async function readJsonFile(file: File): Promise<unknown | null> {
  try {
    return JSON.parse(await file.text());
  } catch {
    return null;
  }
}

export function downloadJson(filename: string, data: unknown) {
  const blob = new Blob([JSON.stringify(data, null, 2)], { type: "application/json" });
  const url = URL.createObjectURL(blob);
  const a = document.createElement("a");
  a.href = url;
  a.download = filename;
  a.click();
  URL.revokeObjectURL(url);
}
