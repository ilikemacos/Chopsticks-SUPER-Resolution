import { Nav } from "@/components/Nav";
import { InstallBanner } from "@/components/InstallBanner";
import { Hero } from "@/components/Hero";
import { Features } from "@/components/Features";
import { CompatTable } from "@/components/CompatTable";
import { Download } from "@/components/Download";
import { Docs } from "@/components/Docs";
import { Faq } from "@/components/Faq";
import { Footer } from "@/components/Footer";

export default function Home() {
  return (
    <>
      <Nav />
      <main>
        <InstallBanner />
        <Hero />
        <Features />
        <CompatTable />
        <Download />
        <Docs />
        <Faq />
      </main>
      <Footer />
    </>
  );
}
