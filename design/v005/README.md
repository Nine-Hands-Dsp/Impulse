# Impulse — design v005 (3600 x 1440, fond transparent)

Rendu réaliste d'après `sources/Impulse_Design_Mockup_v005.svg` : lumière en haut à gauche (45°), ombres portées, occlusion,
alu brossé noir satiné (`sources/texture/`).

- `Impulse_render_3600x1440.png` : rendu complet avec contenus d'exemple
- `Impulse_background_3600x1440.png` : fond statique (sans couronnes de potards, boutons de fader ni contenus d'écran)
- `knob/` : centre fixe, couronne mobile, reflet statique (170 px) et sprite de la couronne (128 images, -135° à +135°)
- `elements/` : boutons poussoirs 120 / 100 px (repos / enfoncé), bouton de fader
- `svg/` : icônes, étiquettes, graduations, logos, calques SVG positionnés (couleur : règle `.c` / `.s` en tête de fichier)
- `sources/build_impulse.py` + `svgkit.py` : régénération (`python3 build_impulse.py`, fichiers d'entrée dans `src/`)
