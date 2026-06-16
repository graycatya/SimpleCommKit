#!/usr/bin/env python3
"""
sync_version.py — 统一版本号管理工具

版本号流向：
  VERSION 文件 (唯一来源)
    → CMake 读取 → C++ #define SIMPLECOMMKIT_VERSION
      → pybind11 get_version() → SimpleCommKitPy 包
        → SimpleCommKitAi 包导入

用法:
  python scripts/sync_version.py              # 同步，使用当前 VERSION
  python scripts/sync_version.py 1.0.0        # 更新 VERSION 并重新构建生效
"""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent


def get_version():
    vf = ROOT / "VERSION"
    if vf.exists():
        return vf.read_text().strip()
    return "0.1.0"


def set_version(ver: str):
    vf = ROOT / "VERSION"
    vf.write_text(ver.strip() + "\n", encoding="utf-8")
    print(f"[OK] VERSION → {ver}")


def main():
    if len(sys.argv) > 1:
        ver = sys.argv[1]
        set_version(ver)
    else:
        ver = get_version()

    print(f"""
╔══════════════════════════════════════════════╗
║  SimpleCommKit 版本: {ver:<22} ║
╠══════════════════════════════════════════════╣
║  版本号来源: VERSION 文件 (唯一来源)         ║
║  C++ 层:   CMake 读取 → version.h            ║
║  Py 包:    pybind11 get_version()            ║
║  Ai 包:    from SimpleCommKitPy* import      ║
╠══════════════════════════════════════════════╣
║  修改版本号后需重新 CMake + 编译:             ║
║    cmake -B build ...                        ║
║    cmake --build build                       ║
╚══════════════════════════════════════════════╝
""")


if __name__ == "__main__":
    main()
