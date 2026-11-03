# Impulse (Studio Wiga)

Chargeur de réponses impulsionnelles (IR) double **A / B** pour amplis guitare et basse, VST3 Windows et macOS (AU en plus sur macOS).

## Fonctions
- Deux slots A / B : bouton dossier, IR précédente / suivante du même dossier, glisser-déposer (deux fichiers déposés ensemble = A et B), clic droit (charger, vider, afficher dans le dossier).
- Formes d'onde, infos du fichier (durée, fréquence, résolution, mono / stéréo).
- Par slot : niveau (+12 dB à -inf), Mute, inversion de phase.
- **MIX** : fondu A / B (au centre, A et B à -6 dB chacun ; un seul slot actif = 100 %).
- **OVERLOAD** : surcharge du haut-parleur (saturation asymétrique, anti-repliement ADAA, sans latence ; 0 % = signal intact).
- **VARIPHI** : décalage fin de B (0 à 1 ms) pour régler la phase entre les deux micros.
- **Couleur** (roue dentée > Colour..., ou clic droit sur le fond) : nuancier d'Octopus (couleurs rapides, MY COLOURS, teinte / saturation / luminosité, code hexadécimal à taper, Default = bleu d'origine). La couleur est enregistrée dans le projet et reprise par les nouvelles fenêtres.
- **Dossier des IR par défaut** (roue dentée > IR folder) : le bouton dossier et les flèches d'un slot vide partent de ce dossier ; un slot chargé utilise le dossier de son IR.
- Roue dentée : normalisation, coupe-bas, coupe-haut, MIDI, taille de la fenêtre.
- Presets (`%APPDATA%\Studio Wiga\Impulse\Presets` / `~/Library/Application Support/Studio Wiga/Impulse/Presets`) : les IR sont copiées dans le preset et dans le projet (≤ 4 s), le son revient même si le fichier a été déplacé.
- MIDI Learn sur toutes les commandes (clic droit), aucun CC par défaut ; footswitch à accrochage ou momentané ; course de pédale apprise automatiquement, CC 14 bits.

## Moteur audio (d'après NAM et TONE3000, licences MIT)
- Convolution partitionnée JUCE sans latence (comme le plugin TONE3000), tête de 4096 points, IR jusqu'à 10 s, fondu interne au changement d'IR.
- Rééchantillonnage des IR en sinc fenêtré (Kaiser, 24 passages par zéro) à la fréquence de l'hôte, réponse en fréquence conservée (plus précis que l'interpolation cubique de NAM et que le `ResamplingAudioSource` de JUCE).
- Normalisation à énergie unité (méthode TONE3000, corrigée de la fréquence d'échantillonnage) : les IR sont au même niveau ; -6 dB pour une IR de baffle (sortie ≈ entrée sur un signal d'ampli), sans pad pour une IR longue (> 1 s, réverbération). Normalisation désactivée : loi NAM (IR brute à -18 dB).
- Silence de fin retiré ; le début est gardé pour ne pas casser l'alignement de phase des IR A / B.
- IR stéréo = stéréo vraie ; piste mono -> sortie stéréo acceptée.
- Latence déclarée : 2 échantillons.

## Anti-gel v2
JUCE 8.0.15, rendu logiciel sous Windows, aucun calcul lourd dans `paint()` (fond calculé par le timer), chien de garde, aucun appel à l'hôte depuis le thread audio (file vidée par un timer à 60 Hz), pas de `setSize` dans `resized()`.

## Build
GitHub Actions (`.github/workflows/build.yml`) : artefacts **Impulse-VST3-Windows** et **Impulse-VST3-macOS**.
Outil Linux de vérification (captures et tests audio hors ligne) : `cmake -B build -DIMPULSE_TOOLS=ON`.

Polices Inter (SIL Open Font License) incluses.
