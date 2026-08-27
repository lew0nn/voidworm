# Third-Party Notices

VOIDWORM original source code and original project assets are Copyright © 2026 lewonn / LWNX DSP and are licensed under the GNU Affero General Public License, version 3.0 (`AGPL-3.0-only`).

The VOIDWORM licence does not replace or modify the licences of third-party components. Each third-party component remains subject to its own copyright notices and licence terms.

## JUCE

VOIDWORM uses JUCE 8.0.8 as a Git submodule:

- Repository: <https://github.com/juce-framework/JUCE>
- Pinned commit: `d6181bde38d858c283c3b7bf699ce6340c050b5d`
- Path: `vendor/JUCE`
- Licence information: <https://github.com/juce-framework/JUCE/blob/8.0.8/LICENSE.md>

JUCE Framework modules are offered by their copyright holder under the GNU Affero General Public License v3 and a separate commercial JUCE licence. VOIDWORM uses the AGPLv3 option. JUCE is not relicensed by VOIDWORM or LWNX DSP.

JUCE contains or integrates additional third-party components under their own terms, including Apache-2.0, BSD, ISC, MIT, zlib, public-domain, and other component-specific licences. Their authoritative notices are retained in the pinned JUCE submodule.

Clone the repository with `--recurse-submodules`, or run `git submodule update --init --recursive`, to obtain the pinned JUCE source and its complete notices.

## Build and platform components

VOIDWORM's build may use SDKs, compiler runtime libraries, system libraries, and packaging tools supplied separately by their respective vendors. Those components are not relicensed by VOIDWORM and are not included in the AGPL grant except where their own licences expressly provide otherwise.
