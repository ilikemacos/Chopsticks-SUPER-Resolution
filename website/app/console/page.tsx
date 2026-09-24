import type { Metadata } from "next";
import { Nav } from "@/components/Nav";
import { Footer } from "@/components/Footer";
import { Console } from "@/components/console/Console";

export const metadata: Metadata = {
  title: "Console",
  description:
    "Scan your system, configure Universal Upscaling, manage per-game profiles, and review inspection and benchmark reports from the Universal FrameFX desktop app.",
};

export default function ConsolePage() {
  return (
    <>
      <Nav />
      <main className="relative overflow-hidden">
        <div className="orb float-a left-[-180px] top-[-220px] h-[420px] w-[420px] bg-accent/20" />
        <div className="orb float-b right-[-200px] top-[120px] h-[380px] w-[380px] bg-good/8" />
        <div className="relative z-10">
          <Console />
        </div>
      </main>
      <Footer />
    </>
  );
}
