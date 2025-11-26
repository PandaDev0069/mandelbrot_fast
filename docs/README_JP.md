# 高性能マンデルブロ集合エクスプローラー

![Mandelbrot](https://img.shields.io/badge/Render-OpenGL-green) ![Precision](https://img.shields.io/badge/Precision-128--bit-blue) ![Acceleration](https://img.shields.io/badge/Acceleration-AVX2%20%2B%20OpenMP-orange)

[🇺🇸 English](README.md) | [🇯🇵 日本語](README_JP.md)

極めて高いズームレベル（$10^{30}$以上）でのリアルタイムレンダリングが可能な、最先端のマンデルブロ集合エクスプローラーです。このプロジェクトでは、**摂動論 (Perturbation Theory)**、**級数近似 (Series Approximation)**、および **AVX2 SIMD** 命令を活用し、標準的な浮動小数点演算の限界を克服しています。

## 🚀 機能

- **超高倍率ズーム**: $10^{30}$ を超える深さまで、完璧な精度で探索可能。
- **ハイブリッド精度エンジン**: 以下を自動的に切り替えます：
  - `double` (64-bit): 浅いズームでの高速化。
  - `long double` (80-bit): 中程度の精度。
  - **摂動論** (128-bit 参照 + 64-bit 差分): 深いズーム用。
- **高性能**:
  - **AVX2 ベクトル化**: 1サイクルあたり4ピクセルを処理。
  - **OpenMP 並列化**: 全CPUコアを使用したマルチスレッドレンダリング。
  - **級数近似 (BLA)**: 深いズームにおいて、最大80%の反復計算をスキップ。
- **滑らかな可視化**:
  - OpenGLベースのレンダリング。
  - 動的なヒストグラム正規化による連続的で滑らかなカラーリング。
  - 高品質なレンダラーに匹敵するパレット。

## 🛠️ インストールと使用法

### 前提条件

- Python 3.x
- 必要なPythonパッケージ: `glfw`, `PyOpenGL`, `numpy`, `matplotlib`
- Windows (コンパイル済みDLL同梱) または コンパイル用のGCC。

### エクスプローラーの実行

Pythonスクリプトを実行するだけです：

```bash
python smooth_mandelbrot.py
```

### 操作方法

- **マウスホイール**: カーソル位置でのズームイン/アウト。
- **左クリック**: ビューをカーソル位置にセンタリング。
- **ESC**: アプリケーションの終了。

## 🔧 技術詳細

使用されている数学的理論や最適化技術（摂動論や2変数線形近似を含む）の詳細については、[最終レポート](FINAL_REPORT_JP.md) を参照してください。

### コンパイル（オプション）

Cバックエンドを再コンパイルする必要がある場合：

```bash
gcc -shared -o mandelbrot_compute.dll mandelbrot_compute.c -O3 -march=native -fopenmp -lquadmath
```

## 📄 ライセンス

このプロジェクトはオープンソースです。自由に探索・修正してください。
