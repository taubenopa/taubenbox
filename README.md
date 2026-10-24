# Tauben-Box 🕊️

VST3-Effekt-Plugin von **taubenopa** (nur Windows). Kurvengesteuerte, layerbare Effekte im Stil von ShaperBox,
dazu ein Granulator, viele Presets und tauben-inspiriertes Design.

## Module (pro Layer wählbar, 4 Layer in Serie)
| Name | Funktion |
|------|----------|
| Flügelschlag | Volume / Gate / Sidechain-Pump |
| Federn | Filter (LP–BP–HP, Cutoff, Resonanz) |
| Kropf | Drive / Sättigung |
| Krümel | Bitcrusher + Downsampling |
| Schwingen | Auto-Pan |
| Spreizen | Stereo-Breite |
| Schwarm | Granulator (Korngröße, Dichte, Tonhöhe) |

Jeder Layer hat eine zeichenbare Kurve, Tempo-Sync (4 Takte bis 1/32 inkl. Triolen), Glättung, Phase,
Mix und optional MIDI-Retrigger.

## Bauen über GitHub
1. Neues Repo auf GitHub anlegen und diesen Ordner hochladen (`git init`, `git add .`, `git commit`, `git push`).
2. Unter **Actions** läuft "Tauben-Box bauen" automatisch (Windows, Visual Studio 2022, JUCE wird per CMake geholt).
3. Den fertigen Installer findest du danach unter *Actions → Lauf → Artifacts → Tauben-Box-Installer*.
4. Für ein offizielles Release einen Tag setzen: `git tag v0.1.0 && git push --tags`. Der Installer hängt dann am Release.

## Lokal bauen
```
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release
```
Das Plugin liegt danach unter `build/TaubenBox_artefacts/Release/VST3/Tauben-Box.vst3`.

## Installer
`installer/TaubenBox.iss` (Inno Setup) mit Tauben-Grafiken aus `installer/assets/`.
Die Grafiken lassen sich mit `python installer/make_installer_art.py` neu erzeugen (benötigt Pillow).
