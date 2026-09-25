"""PSNR and SSIM, so quality claims are measured rather than asserted."""
from __future__ import annotations

import numpy as np


def psnr(a: np.ndarray, b: np.ndarray) -> float:
    mse = float(np.mean((a.astype(np.float64) - b.astype(np.float64)) ** 2))
    if mse <= 0:
        return float("inf")
    return 10.0 * np.log10(1.0 / mse)


def _gauss(n: int = 11, sigma: float = 1.5) -> np.ndarray:
    x = np.arange(n, dtype=np.float64) - (n - 1) / 2
    k = np.exp(-(x ** 2) / (2 * sigma ** 2))
    return k / k.sum()


def _blur(img: np.ndarray, k: np.ndarray) -> np.ndarray:
    pad = len(k) // 2
    p = np.pad(img, ((pad, pad), (pad, pad)), mode="reflect")
    out = np.empty_like(img, dtype=np.float64)
    tmp = np.zeros((p.shape[0], img.shape[1]), dtype=np.float64)
    for i, w in enumerate(k):
        tmp += w * p[:, i:i + img.shape[1]]
    out[:] = 0.0
    for i, w in enumerate(k):
        out += w * tmp[i:i + img.shape[0], :]
    return out


def ssim(a: np.ndarray, b: np.ndarray) -> float:
    """Mean SSIM on luma, Gaussian-windowed (Wang et al. defaults)."""
    la = a[..., 1].astype(np.float64)
    lb = b[..., 1].astype(np.float64)
    k = _gauss()
    C1, C2 = 0.01 ** 2, 0.03 ** 2
    mu_a, mu_b = _blur(la, k), _blur(lb, k)
    va = _blur(la * la, k) - mu_a * mu_a
    vb = _blur(lb * lb, k) - mu_b * mu_b
    cab = _blur(la * lb, k) - mu_a * mu_b
    num = (2 * mu_a * mu_b + C1) * (2 * cab + C2)
    den = (mu_a ** 2 + mu_b ** 2 + C1) * (va + vb + C2)
    return float(np.mean(num / den))
