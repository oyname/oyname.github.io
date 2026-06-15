---
title: Minimal Window
image: assets/helloworld.png
download: files/minimal_window.cpp
downloadLabel: Download
---
Open a window and run a minimal per-frame loop with the high-level `Krom::` wrapper on the OpenGL backend. Polls the keyboard each frame and closes on Escape. The simplest possible entry point into KROM.

Covers `Krom::Graphics`, `Krom::Run([](float dt){ ... })`, and input polling via `Krom::KeyHit`, `Krom::KeyDown`, and `Krom::Quit`.
