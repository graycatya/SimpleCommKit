"""Tests for SimpleCommKitAiHid."""

import os as _sc_os
_sc_dir = _sc_os.path.dirname(_sc_os.path.abspath(__file__))
for _ in range(10):
    _sc_vf = _sc_os.path.join(_sc_dir, "VERSION")
    if _sc_os.path.isfile(_sc_vf):
        with open(_sc_vf, "r") as _sc_f:
            __version__ = _sc_f.read().strip()
        break
    _sc_parent = _sc_os.path.dirname(_sc_dir)
    if _sc_parent == _sc_dir:
        __version__ = "0.0.0"
        break
    _sc_dir = _sc_parent
else:
    __version__ = "0.0.0"
