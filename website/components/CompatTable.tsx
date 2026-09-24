import { compatRows, supportLabel, type Support } from "@/lib/compat";

function Cell({ value }: { value: Support }) {
  const styles: Record<Support, string> = {
    yes: "text-good",
    no: "text-bad",
    game: "text-warn",
    partial: "text-warn",
  };
  const symbol: Record<Support, string> = {
    yes: "●",
    no: "○",
    game: "◐",
    partial: "◐",
  };
  return (
    <span className={`inline-flex items-center gap-1.5 ${styles[value]}`}>
      <span aria-hidden>{symbol[value]}</span>
      <span className="text-xs text-muted">{supportLabel[value]}</span>
    </span>
  );
}

export function CompatTable() {
  return (
    <section id="compatibility" className="mx-auto max-w-6xl px-4 py-24 sm:px-6">
      <div className="mx-auto max-w-2xl text-center">
        <p className="section-label">Straight answers</p>
        <h2 className="mt-3 font-display text-4xl font-bold tracking-tight text-gradient sm:text-5xl">
          Compatibility
        </h2>
        <p className="mx-auto mt-4 text-muted">
        Populated only with technically verified, vendor-documented information.
        &ldquo;Game integration&rdquo; means the technology must be built into the
        game engine and cannot be added externally.
        </p>
      </div>

      <div className="glass mt-10 overflow-x-auto rounded-2xl">
        <table className="w-full min-w-[720px] border-collapse text-sm">
          <thead>
            <tr className="border-b border-white/10 bg-white/[0.03] text-left">
              <th className="p-4 font-medium">Technology</th>
              <th className="p-4 font-medium">DX11</th>
              <th className="p-4 font-medium">DX12</th>
              <th className="p-4 font-medium">NVIDIA</th>
              <th className="p-4 font-medium">AMD</th>
              <th className="p-4 font-medium">Intel</th>
            </tr>
          </thead>
          <tbody>
            {compatRows.map((row) => (
              <tr key={row.tech} className="border-t border-white/5 align-top">
                <td className="p-4">
                  <div className="font-medium">{row.tech}</div>
                  <div className="mt-1 max-w-xs text-xs text-muted">{row.note}</div>
                </td>
                <td className="p-4"><Cell value={row.dx11} /></td>
                <td className="p-4"><Cell value={row.dx12} /></td>
                <td className="p-4"><Cell value={row.nvidia} /></td>
                <td className="p-4"><Cell value={row.amd} /></td>
                <td className="p-4"><Cell value={row.intel} /></td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      <div className="mt-4 flex flex-wrap gap-4 text-xs text-muted">
        <span><span className="text-good">●</span> Yes</span>
        <span><span className="text-warn">◐</span> Partial / game integration</span>
        <span><span className="text-bad">○</span> No</span>
      </div>
    </section>
  );
}
