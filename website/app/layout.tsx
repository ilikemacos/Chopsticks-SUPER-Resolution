import type { Metadata } from "next";
import "./globals.css";

const siteUrl = "https://universalframefx.example";

export const metadata: Metadata = {
  metadataBase: new URL(siteUrl),
  title: {
    default: "Universal FrameFX — FSR, XeSS & Frame Generation, one interface",
    template: "%s — Universal FrameFX",
  },
  description:
    "Open-source Windows 11 app to configure FidelityFX Super Resolution (FSR/FSR 3/FSR 4), Intel XeSS and frame generation for the games you already own. Honest about what each technology can and cannot do.",
  keywords: [
    "FSR", "FSR 3", "FSR 4", "XeSS", "frame generation", "upscaling",
    "Windows 11", "open source", "FidelityFX", "DirectX 12",
  ],
  authors: [{ name: "Universal FrameFX contributors" }],
  openGraph: {
    type: "website",
    url: siteUrl,
    title: "Universal FrameFX",
    description:
      "Universal graphics enhancement for Windows 11. FSR. XeSS. Frame Generation. One interface.",
    siteName: "Universal FrameFX",
  },
  twitter: {
    card: "summary_large_image",
    title: "Universal FrameFX",
    description:
      "Open-source Windows 11 app for FSR, XeSS and frame generation. No fake support claims.",
  },
  robots: { index: true, follow: true },
};

export default function RootLayout({
  children,
}: {
  children: React.ReactNode;
}) {
  return (
    <html lang="en">
      <body className="min-h-screen antialiased">{children}</body>
    </html>
  );
}
