# IMPULSE (NINE HANDS | DSP) - notice PDF bilingue (anglais puis francais).
# Usage : python3 tools/manual.py [fichier PDF de sortie]
# Version lue dans CMakeLists.txt. Illustrations : tools/manual_assets/ (rendus reels de l'interface, outil tools/ImpulseTool).
import os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import manual_lib as L

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSION = re.search(r"project\s*\(\s*\w+\s+VERSION\s+([0-9.]+)", open(os.path.join(ROOT, "CMakeLists.txt")).read()).group(1)
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "docs", "Impulse_Manual_EN-FR_v" + VERSION + ".pdf")

TEXTS = {"en": dict(lang="ENGLISH", sub=["Dual A/B impulse response loader", "for guitar and bass amps", "VST3 for Windows and macOS"],
                    ver="User manual \u00b7 version " + VERSION),
         "fr": dict(lang="FRANÇAIS", sub=["Chargeur de réponses impulsionnelles A/B", "pour amplis guitare et basse", "VST3 pour Windows et macOS"],
                    ver="Notice d'utilisation \u00b7 version " + VERSION)}


def content(lang):
    en = lang == "en"
    def t(a, b): return a if en else b
    b = []
    # 1. Presentation
    b += [("h1", t("1. Introduction", "1. Présentation")),
          ("p", t("<b>Impulse</b> loads <b>impulse responses</b> (IRs): short audio files that capture how a speaker cabinet and its microphone colour the sound. Placed after an amp simulator (with its own cab turned off), a NAM capture or a preamp pedal, it turns the raw amp signal into a miked cabinet, for guitar and bass.",
                  "<b>Impulse</b> charge des <b>réponses impulsionnelles</b> (IR) : de courts fichiers audio qui capturent la façon dont un baffle et son micro colorent le son. Placé après un simulateur d'ampli (baffle désactivé), une capture NAM ou une pédale de préampli, il transforme le signal brut de l'ampli en baffle repris au micro, pour guitare et basse.")),
          ("p", t("Two slots, <b>A</b> and <b>B</b>, can be loaded at the same time and blended, like two microphones on the same cabinet: MIX balances them, VARIPHI shifts the phase of B, and each slot has its own level, mute and phase invert.",
                  "Deux emplacements, <b>A</b> et <b>B</b>, se chargent en même temps et se mélangent, comme deux micros sur le même baffle : MIX les dose, VARIPHI décale la phase de B, et chaque emplacement a son niveau, sa coupure et son inversion de phase.")),
          ("p", t("The audio engine follows the open-source <b>TONE3000</b> plugin and <b>Neural Amp Modeler (NAM)</b> (both MIT licence): zero-latency partitioned convolution, unit-energy level matching and the -18 dB gain law. Impulse goes further on resampling: an IR recorded at another sample rate is converted with a windowed-sinc (Kaiser) resampler, so its frequency response stays within 0.1 dB of the original.",
                  "Le moteur audio suit le plugin open source <b>TONE3000</b> et <b>Neural Amp Modeler (NAM)</b> (licence MIT tous les deux) : convolution partitionnée sans latence, égalisation des niveaux à énergie unité et loi de gain à -18 dB. Impulse va plus loin sur le rééchantillonnage : une IR enregistrée à une autre fréquence est convertie par un rééchantillonneur sinc fenêtré (Kaiser), sa réponse en fréquence reste à moins de 0,1 dB de l'originale.")),
          ("p", t("Impulse <b>accepts MIDI</b>: every control can be learned with a right-click (section 6). No CC is assigned by default.",
                  "Impulse <b>accepte le MIDI</b> : chaque commande s'apprend par clic droit (section 6). Aucun CC n'est assigné par défaut."))]
    # 2. Installation
    b += [("h1", "2. Installation"),
          ("h2", "Windows"),
          ("p", t("Download the <b>Impulse-VST3-Windows</b> artifact (GitHub Actions, repository Nine-Hands-Dsp/Impulse), unzip it and copy <b>Impulse.vst3</b> into <b>C:\\Program Files\\Common Files\\VST3\\</b>.",
                  "Téléchargez l'artefact <b>Impulse-VST3-Windows</b> (GitHub Actions, dépôt Nine-Hands-Dsp/Impulse), décompressez-le et copiez <b>Impulse.vst3</b> dans <b>C:\\Program Files\\Common Files\\VST3\\</b>.")),
          ("h2", "macOS"),
          ("p", t("Download <b>Impulse-VST3-macOS</b> (universal: Apple Silicon and Intel, macOS 10.13+), unzip it (it contains another zip), unzip again and copy <b>Impulse.vst3</b> into <b>~/Library/Audio/Plug-Ins/VST3/</b>. The plugin is not notarized: run once in Terminal <b>xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Impulse.vst3</b>",
                  "Téléchargez <b>Impulse-VST3-macOS</b> (universel : Apple Silicon et Intel, macOS 10.13+), décompressez-le (il contient un autre zip), décompressez encore et copiez <b>Impulse.vst3</b> dans <b>~/Library/Audio/Plug-Ins/VST3/</b>. Le plugin n'est pas notarié : lancez une fois dans le Terminal <b>xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Impulse.vst3</b>")),
          ("h2", "Reaper"),
          ("p", t("Options &gt; Preferences &gt; Plug-ins &gt; VST &gt; <b>Re-scan</b>. Impulse is listed as <b>Impulse (NINE HANDS | DSP)</b>. For MIDI, the track must send MIDI to the plugin: record-arm it with your MIDI controller as input, or use a MIDI send.",
                  "Options &gt; Preferences &gt; Plug-ins &gt; VST &gt; <b>Re-scan</b>. Impulse apparaît sous le nom <b>Impulse (NINE HANDS | DSP)</b>. Pour le MIDI, la piste doit envoyer le MIDI au plugin : armez-la avec votre contrôleur MIDI en entrée, ou utilisez un envoi MIDI.")),
          ("pb",)]
    # 3. Overview
    rows = [["#", t("Control", "Commande"), t("Role", "Rôle")],
            ["1", "Power", t("Effect on / off (smooth crossfade with the dry signal)", "Effet actif / coupé (fondu doux avec le son direct)")],
            ["2", t("Preset menu", "Menu des presets"), t("Arrows: previous / next preset; click the name: preset list and options", "Flèches : preset précédent / suivant ; clic sur le nom : liste et options")],
            ["3", t("Preset file", "Fichier preset"), t("Save, Save as, Export to file, Import from file, presets folder", "Save, Save as, export vers un fichier, import depuis un fichier, dossier des presets")],
            ["4", t("Menu", "Menu"), t("Plugin size, Settings, MIDI (the right-click menu without Presets)", "Plugin size, Settings, MIDI (le menu du clic droit sans Presets)")],
            ["5", t("IR name", "Nom de l'IR"), t("Click: load a file; drop a file here; right-click: slot menu", "Clic : charger un fichier ; déposez un fichier ici ; clic droit : menu du slot")],
            ["6", t("Folder", "Dossier"), t("Opens the file browser", "Ouvre le sélecteur de fichiers")],
            ["7", t("Previous / next", "Précédente / suivante"), t("Loads the previous / next IR of the same folder", "Charge l'IR précédente / suivante du même dossier")],
            ["8", t("Waveform", "Forme d'onde"), t("The IR as it is convolved, with length, sample rate, bit depth, mono / stereo", "L'IR telle qu'elle est utilisée, avec durée, fréquence, résolution, mono / stéréo")],
            ["9", "M", t("Mutes the slot", "Coupe le slot")],
            ["10", "\u00d8", t("Inverts the polarity of the slot", "Inverse la polarité du slot")],
            ["11", t("Level fader", "Fader de niveau"), t("+12 dB to -\u221e, 0 dB by default", "+12 dB à -\u221e, 0 dB par défaut")],
            ["12", "MIX", t("Balance between IR A and IR B", "Dosage entre l'IR A et l'IR B")],
            ["13", "OVERLOAD", t("Speaker cone overload (saturation before the IRs)", "Surcharge du haut-parleur (saturation avant les IR)")],
            ["14", "VARIPHI", t("Fine delay of B (0 to 1 ms): phase between the two mics", "Retard fin de B (0 à 1 ms) : phase entre les deux micros")],
            ["15", t("Status line", "Ligne d'état"), t("Value of the control being moved, MIDI Learn, messages", "Valeur de la commande qui bouge, MIDI Learn, messages")]]
    b += [("h1", t("3. Interface overview", "3. Vue d'ensemble de l'interface")),
          ("img", "annotated.png", 168, t("Slot B (right) works exactly like slot A.", "Le slot B (à droite) fonctionne exactement comme le slot A.")),
          ("table", rows, [8, 34, None], {"small": True})]
    # 4. Controls
    b += [("cond", 60), ("h1", t("4. Controls", "4. Les commandes")),
          ("h2", t("Loading IRs (slots A and B)", "Charger des IR (slots A et B)")),
          ("bullets", [t("<b>Formats</b>: WAV, AIFF, FLAC, 16 / 24 / 32-bit, any sample rate, mono or stereo (a file with more channels uses its first two). Up to 10 seconds: cabinet IRs, but also room or reverb IRs.",
                         "<b>Formats</b> : WAV, AIFF, FLAC, 16 / 24 / 32 bits, toute fréquence, mono ou stéréo (un fichier à plus de canaux utilise ses deux premiers). Jusqu'à 10 secondes : IR de baffle, mais aussi de pièce ou de reverb."),
                       t("<b>Load</b>: click the name box or the folder button, or <b>drag and drop</b> a file onto a slot. Drop <b>two files at once</b>: the first goes into A, the second into B. Dropping a folder loads its first IR.",
                         "<b>Charger</b> : cliquez sur le cadre du nom ou sur le bouton dossier, ou <b>glissez-déposez</b> un fichier sur un slot. Déposez <b>deux fichiers d'un coup</b> : le premier va dans A, le second dans B. Déposer un dossier charge sa première IR."),
                       t("<b>Default IR folder</b>: right-click or gear &gt; Settings &gt; IR folder &gt; Choose default IR folder... sets where your IR library lives (section 5). When a slot already holds an IR, the folder button and the arrows use the folder of that IR.",
                         "<b>Dossier des IR par défaut</b> : clic droit ou roue dentée &gt; Settings &gt; IR folder &gt; Choose default IR folder... indique où se trouve votre bibliothèque d'IR (section 5). Quand un slot contient déjà une IR, le bouton dossier et les flèches utilisent le dossier de cette IR."),
                       t("<b>Browse</b>: the arrows load the previous / next IR of the same folder (natural order, wrapping around). The switch is a smooth crossfade, without click: ideal to compare mics while playing.",
                         "<b>Parcourir</b> : les flèches chargent l'IR précédente / suivante du même dossier (ordre naturel, en boucle). Le changement se fait en fondu, sans clic : idéal pour comparer des micros en jouant."),
                       t("<b>Right-click</b> on a name or a waveform: Load IR, Previous / Next IR in folder, Show in folder, Clear, and the gain applied to this IR.",
                         "<b>Clic droit</b> sur un nom ou une forme d'onde : Load IR, IR précédente / suivante du dossier, Show in folder, Clear, et le gain appliqué à cette IR."),
                       t("A file that cannot be read shows a red message for a few seconds; the IR already loaded stays.",
                         "Un fichier illisible affiche un message rouge quelques secondes ; l'IR déjà chargée reste en place.")]),
          ("h2", t("Level, M and \u00d8", "Niveau, M et \u00d8")),
          ("p", t("Each slot has a fader from <b>+12 dB to -\u221e</b> (double-click: back to 0 dB; Shift + drag: fine), a <b>M</b> button to mute it (its waveform dims) and a <b>\u00d8</b> button that inverts its polarity, useful when two mics cancel each other.",
                  "Chaque slot a un fader de <b>+12 dB à -\u221e</b> (double-clic : retour à 0 dB ; Maj + glisser : réglage fin), un bouton <b>M</b> qui le coupe (sa forme d'onde s'assombrit) et un bouton <b>\u00d8</b> qui inverse sa polarité, utile quand deux micros s'annulent.")),
          ("h2", "MIX"),
          ("p", t("The knob shows the amount and the side that dominates: <b>50A</b> = halfway towards A (A 75 %, B 25 %), <b>30B</b> = towards B, <b>0</b> = centre. A small <b>detent</b> holds the knob on 0 when you cross from one side to the other (with the mouse wheel too); double-click anywhere on the knob, value included, to go back to the centre.",
                  "Le potard affiche la quantité et le côté qui domine : <b>50A</b> = à mi-course vers A (A 75 %, B 25 %), <b>30B</b> = vers B, <b>0</b> = centre. Une petite <b>butée</b> retient le potard sur 0 quand on passe d'un côté à l'autre (à la molette aussi) ; un double-clic n'importe où sur le potard, valeur comprise, le remet au centre.")),
          ("p", t("From <b>100A</b> (only A) to <b>100B</b> (only B), <b>0</b> = both at -6 dB each, a linear crossfade that keeps the level constant for two IRs of the same cabinet. When only one slot is active (the other empty or muted), it plays at 100 % whatever MIX says. With no IR at all, the sound goes through untouched.",
                  "De <b>100A</b> (A seul) à <b>100B</b> (B seul), <b>0</b> = les deux à -6 dB chacun, un fondu linéaire qui garde le niveau constant pour deux IR du même baffle. Quand un seul slot est actif (l'autre vide ou coupé), il joue à 100 % quel que soit MIX. Sans aucune IR, le son passe sans traitement.")),
          ("h2", "OVERLOAD"),
          ("p", t("Simulates a speaker pushed beyond its limits, before the IRs: asymmetric saturation (even harmonics) with up to +24 dB of drive and partial level compensation. The saturation is anti-aliased (ADAA) without oversampling, so it adds no latency, and at <b>0 %</b> the signal is strictly unchanged. A touch (10 to 30 %) adds compression and bloom; higher settings give a broken-up, crunchy cab.",
                  "Simule un haut-parleur poussé au-delà de ses limites, avant les IR : saturation asymétrique (harmoniques paires) jusqu'à +24 dB d'attaque avec compensation partielle du niveau. La saturation est anti-repliement (ADAA) sans suréchantillonnage, donc sans latence, et à <b>0 %</b> le signal est strictement inchangé. Un peu (10 à 30 %) ajoute compression et épaisseur ; plus haut, le baffle se met à saturer.")),
          ("h2", "VARIPHI"),
          ("p", t("Delays IR B by <b>0 to 1 ms</b> (100 % = 1 ms, about 34 cm of mic distance), with a smooth fractional delay. Between two mics this moves the comb filtering: 0.5 ms puts a notch at 1 kHz. Turn it slowly with both IRs at MIX 0 to find the fullest or the most hollow blend. With only A loaded, VARIPHI does nothing.",
                  "Retarde l'IR B de <b>0 à 1 ms</b> (100 % = 1 ms, environ 34 cm d'écart de micro), avec un retard fractionnaire sans saccade. Entre deux micros, cela déplace le filtrage en peigne : 0,5 ms met un creux à 1 kHz. Tournez-le lentement avec les deux IR à MIX 0 pour trouver le mélange le plus plein ou le plus creux. Avec A seul, VARIPHI n'a aucun effet.")),
          ("h2", t("Knobs", "Potards")),
          ("p", t("Drag up / down (Shift: fine), mouse wheel, double-click: default value. The value is shown in the knob and in the status line.",
                  "Glisser vers le haut / bas (Maj : fin), molette, double-clic : valeur par défaut. La valeur s'affiche dans le potard et dans la ligne d'état."))]
    # 5. Settings
    b += [("h1", t("5. Menu (right-click or gear button)", "5. Menu (clic droit ou roue dentée)")),
          ("p", t("A right-click anywhere opens the same menu as on the other NINE HANDS | DSP plugins; the gear button opens it too, without the Presets sub-menu. On a control, its <b>MIDI Learn</b> and <b>Clear MIDI</b> come first; on a slot (name or waveform), the IR actions come first (Load IR, previous / next IR in folder, Show in folder, Clear, gain applied). Then: <b>Plugin size</b>, <b>Presets</b>, <b>Settings</b>, <b>MIDI</b> and the plugin version. The <b>Settings</b> sub-menu contains:",
                  "Un clic droit n'importe où ouvre le même menu que sur les autres plugins NINE HANDS | DSP ; la roue dentée l'ouvre aussi, sans le sous-menu Presets. Sur une commande, son <b>MIDI Learn</b> et <b>Clear MIDI</b> viennent en premier ; sur un slot (nom ou forme d'onde), les actions de l'IR viennent en premier (Load IR, IR précédente / suivante du dossier, Show in folder, Clear, gain appliqué). Ensuite : <b>Plugin size</b>, <b>Presets</b>, <b>Settings</b>, <b>MIDI</b> et la version du plugin. Le sous-menu <b>Settings</b> contient :")),
          ("table", [[t("Item", "Élément"), t("Role", "Rôle")],
                     ["Colour...", t("Colour picker (section 7)", "Nuancier (section 7)")],
                     ["IR folder", t("Default IR folder, shared by all Impulse windows: the folder button of an empty slot opens there, and the arrows of an empty slot browse it. Choose default IR folder..., Show default IR folder, or No default folder (use the last folder used).",
                                     "Dossier des IR par défaut, commun à toutes les fenêtres d'Impulse : le bouton dossier d'un slot vide s'ouvre dedans, et les flèches d'un slot vide le parcourent. Choose default IR folder..., Show default IR folder, ou No default folder (dernier dossier utilisé).")],
                     ["Normalize IRs", t("On (default): every IR is brought to the same loudness (unit energy, TONE3000 method, corrected for the sample rate), -6 dB for a cabinet IR so that the output is about the input level on an amp signal, no pad for a long IR (reverb, &gt; 1 s). Off: raw IR at -18 dB (NAM law).",
                                         "Activé (par défaut) : chaque IR est ramenée au même niveau (énergie unité, méthode TONE3000, corrigée de la fréquence d'échantillonnage), -6 dB pour une IR de baffle pour que la sortie soit à peu près au niveau de l'entrée sur un signal d'ampli, sans pad pour une IR longue (reverb, &gt; 1 s). Désactivé : IR brute à -18 dB (loi NAM).")],
                     ["Low cut", t("Off, 40, 60, 80, 100, 120, 150 Hz (12 dB/oct, after the mix)", "Off, 40, 60, 80, 100, 120, 150 Hz (12 dB/oct, après le mélange)")],
                     ["High cut", t("Off, 12, 10, 8, 7, 6, 5 kHz (12 dB/oct)", "Off, 12, 10, 8, 7, 6, 5 kHz (12 dB/oct)")],
                     ["MIDI footswitches", t("Latching (127 = on / 0 = off, default) or momentary (each press toggles), for Power, Mute and Phase (section 6)", "À accrochage (127 = on / 0 = off, par défaut) ou momentané (chaque appui inverse), pour Power, Mute et Phase (section 6)")]],
           [32, None]),
          ("p", t("Low cut and high cut are also automatable parameters with a continuous range (20 to 400 Hz, 2 to 20 kHz). The trailing silence of an IR is removed automatically, but its beginning is kept so that two IRs recorded together stay phase-aligned.",
                  "Coupe-bas et coupe-haut sont aussi des paramètres automatisables à plage continue (20 à 400 Hz, 2 à 20 kHz). Le silence de fin d'une IR est retiré automatiquement, mais son début est gardé pour que deux IR enregistrées ensemble restent alignées en phase."))]
    # 6. MIDI
    b += [("cond", 90), ("h1", t("6. MIDI and expression pedals", "6. MIDI et pédales d'expression")),
          ("p", t("<b>Right-click a control</b> &gt; MIDI Learn, then move a knob / pedal or press a footswitch on your controller: the status line confirms “Mix A/B = CC 11”. Right-click &gt; Clear MIDI removes it. A CC number used by another control moves to the new one. <b>No CC is assigned by default</b>; assignments are saved with the project.",
                  "<b>Clic droit sur une commande</b> &gt; MIDI Learn, puis bougez un potard / une pédale ou appuyez sur un footswitch de votre contrôleur : la ligne d'état confirme « Mix A/B = CC 11 ». Clic droit &gt; Clear MIDI l'efface. Un numéro de CC déjà pris passe à la nouvelle commande. <b>Aucun CC n'est assigné par défaut</b> ; les assignations sont sauvegardées avec le projet.")),
          ("table", [[t("Learnable", "Apprenables"), t("Behaviour", "Comportement")],
                     [t("Mix A/B, Overload, Variphi, Level A, Level B", "Mix A/B, Overload, Variphi, Level A, Level B"),
                      t("Continuous. Expression pedal travel learned automatically (a pedal sending only 2-116 still reaches 0 and 100 %), small dead zones at both ends, 14-bit CC accepted, smoothed so there is no zipper noise.",
                        "Continu. Course de la pédale d'expression apprise automatiquement (une pédale qui n'envoie que 2-116 atteint quand même 0 et 100 %), petites zones mortes aux butées, CC 14 bits accepté, lissé sans bruit d'escalier.")],
                     [t("Mute A, Mute B, Phase A, Phase B, Power", "Mute A, Mute B, Phase A, Phase B, Power"),
                      t("Latching footswitch: 127 = on, 0 = off. With Settings &gt; “MIDI footswitches momentary (toggle each press)” every press toggles. Can also learn a MIDI note.",
                        "Footswitch à accrochage : 127 = on, 0 = off. Avec Settings &gt; « MIDI footswitches momentary (toggle each press) » chaque appui inverse. Peut aussi apprendre une note MIDI.")],
                     [t("IR A previous / next, IR B previous / next, Preset previous / next", "IR A précédente / suivante, IR B précédente / suivante, preset précédent / suivant"),
                      t("One step per press (CC &ge; 64 or note): browse cabinets or presets from the floor.",
                        "Un pas par appui (CC &ge; 64 ou note) : parcourez baffles ou presets au pied.")]],
           [52, None]),
          ("p", t("Right-click &gt; MIDI (or gear &gt; MIDI) gives the full list with the current assignments.",
                  "Un clic droit sur une zone vide (ou roue dentée &gt; MIDI) donne la liste complète avec les assignations en cours."))]
    # 7. Window, colour
    b += [("h1", t("7. Window and colour", "7. Fenêtre et couleur")),
          ("p", t("The window opens at the default size, called <b>60 %</b>, and can never be made smaller. Right-click &gt; Plugin size: <b>60 % / 75 % / 100 %</b> for this window, and <b>Default size (new windows)</b> for every new Impulse window. The corner handle enlarges it further, proportions kept. The size of each instance is saved with the project.",
                  "La fenêtre s'ouvre à la taille par défaut, appelée <b>60 %</b>, et ne peut jamais être plus petite. Clic droit &gt; Plugin size : <b>60 % / 75 % / 100 %</b> pour cette fenêtre, et <b>Default size (new windows)</b> pour toutes les nouvelles fenêtres d'Impulse. La poignée du coin l'agrandit encore, proportions conservées. La taille de chaque instance est sauvegardée avec le projet.")),
          ("h2", t("Stack under Ampire (REAPER)", "Empilé sous Ampire (REAPER)")),
          ("p", t("In REAPER, when Ampire and Impulse are on the same track, the <b>IMPULSE</b> button of Ampire (under POWER) shows this Impulse docked right under Ampire, in the same window. Impulse stays at its place in the FX chain and keeps its own window; both views control the same Impulse. Click again to hide it; the choice is saved with the project. If several Impulse are on the track, Ampire shows the first one.",
                  "Dans REAPER, quand Ampire et Impulse sont sur la même piste, le bouton <b>IMPULSE</b> d'Ampire (sous POWER) affiche cet Impulse collé sous Ampire, dans la même fenêtre. Impulse reste à sa place dans la chaîne d'effets et garde sa propre fenêtre ; les deux vues pilotent le même Impulse. Un nouveau clic le masque ; le choix est sauvegardé avec le projet. S'il y a plusieurs Impulse sur la piste, Ampire affiche le premier.")),
          ("side", "picker.png", 62,
           [("h2", t("Accent colour", "Couleur d'accent")),
            ("p", t("Right-click or gear &gt; Settings &gt; <b>Colour...</b> opens the Octopus colour picker: quick colours, <b>MY COLOURS</b> (click an empty slot to save the current colour, right-click to remove), hue / saturation / brightness, and a box where you can <b>type a colour code</b> (e.g. #172633). The colour changes live; DEFAULT gives back the original blue, OK closes.",
                    "Clic droit ou roue dentée &gt; Settings &gt; <b>Colour...</b> ouvre le nuancier d'Octopus : couleurs rapides, <b>MY COLOURS</b> (clic sur un emplacement vide pour enregistrer la couleur, clic droit pour l'effacer), teinte / saturation / luminosité, et une case pour <b>taper un code couleur</b> (ex. #172633). La couleur change en direct ; DEFAULT rend le bleu d'origine, OK ferme.")),
            ("p", t("A code typed in the box is applied with Enter or OK. The colour is saved with the project and becomes the colour of new Impulse windows. Very dark colours make the waveforms hard to see.",
                    "Un code tapé dans la case s'applique avec Entrée ou OK. La couleur est sauvegardée avec le projet et devient celle des nouvelles fenêtres d'Impulse. Une couleur très sombre rend les formes d'onde peu visibles."))]),
          ("img", "colour.png", 92, t("Same interface with an orange accent and a stereo IR in slot B.", "La même interface avec un accent orange et une IR stéréo dans le slot B."))]
    # 8. Presets
    b += [("cond", 70), ("h1", "8. Presets"),
          ("p", t("Impulse has no factory presets (IRs are your own files). The menu at the top (and right-click &gt; Presets) contains <b>Init (empty)</b>, your presets in alphabetical order, <b>Save</b>, <b>Save preset as...</b>, <b>Delete</b>, <b>Export preset to file...</b>, <b>Import preset from file...</b> and <b>Open presets folder</b>; the <b>floppy disk</b> button next to the gear gives the same file actions. The arrows on the left of the menu load the previous / next preset. A <b>*</b> after the name means the setting has changed since the preset was loaded; “(Custom)” means no preset is loaded.",
                  "Impulse n'a pas de presets d'usine (les IR sont vos propres fichiers). Le menu du haut (et clic droit &gt; Presets) contient <b>Init (empty)</b>, vos presets par ordre alphabétique, <b>Save</b>, <b>Save preset as...</b>, <b>Delete</b>, <b>Export preset to file...</b>, <b>Import preset from file...</b> et <b>Open presets folder</b> ; la <b>disquette</b> à côté de la roue dentée donne les mêmes actions de fichier. Les flèches à gauche du menu chargent le preset précédent / suivant. Une <b>*</b> après le nom signale un réglage modifié depuis le chargement ; « (Custom) » signifie qu'aucun preset n'est chargé.")),
          ("p", t("A preset stores every parameter and both IRs: the file path, plus a <b>copy of the IR</b> (up to 4 s) inside the preset, so it still sounds right if the file has been moved or renamed (the info line then says “saved copy”). Projects store the same copy. MIDI assignments stay with the instance, not with the preset.",
                  "Un preset garde tous les paramètres et les deux IR : le chemin du fichier, plus une <b>copie de l'IR</b> (jusqu'à 4 s) dans le preset, qui sonne donc encore juste si le fichier a été déplacé ou renommé (la ligne d'infos affiche alors « saved copy »). Les projets gardent la même copie. Les assignations MIDI restent liées à l'instance, pas au preset.")),
          ("p", t("<b>Export</b> writes the current setting, IRs included, to a .impulse file wherever you want (to back it up or send it to another computer). <b>Import</b> loads a .impulse file and copies it into your presets list; if the name is already taken, a number is added instead of overwriting.",
                  "<b>Export</b> écrit le réglage en cours, IR comprises, dans un fichier .impulse où vous voulez (sauvegarde, autre ordinateur). <b>Import</b> charge un fichier .impulse et le copie dans votre liste de presets ; si le nom existe déjà, un numéro est ajouté au lieu d'écraser.")),
          ("table", [["", t("Presets folder", "Dossier des presets")],
                     ["Windows", "%APPDATA%\\NINE HANDS DSP\\Impulse\\Presets\\*.impulse"],
                     ["macOS", "~/Library/Application Support/NINE HANDS DSP/Impulse/Presets/*.impulse"]],
           [24, None])]
    # 9. Tips
    b += [("h1", t("9. Tips", "9. Conseils d'utilisation")),
          ("bullets", [t("<b>Chain in Reaper</b>: guitar &gt; your NINE HANDS | DSP pedals &gt; amp (Ampire with a NAM capture, Anode, a preamp) &gt; <b>Impulse</b> &gt; reverb / delay. An IR loader always goes after the amp.",
                         "<b>Chaîne dans Reaper</b> : guitare &gt; vos pédales NINE HANDS | DSP &gt; ampli (Ampire avec une capture NAM, Anode, préampli) &gt; <b>Impulse</b> &gt; reverb / delay. Un chargeur d'IR se place toujours après l'ampli."),
                       t("<b>Two mics</b>: load the SM57 and the ribbon (or condenser) of the same pack in A and B, MIX at 0, then turn VARIPHI slowly; if the blend sounds thin, try \u00d8 on B.",
                         "<b>Deux micros</b> : chargez le SM57 et le ruban (ou statique) du même pack dans A et B, MIX à 0, puis tournez VARIPHI lentement ; si le mélange sonne maigre, essayez \u00d8 sur B."),
                       t("<b>7-string in G and bass</b>: keep Low cut on Off or 40 Hz to keep the low end; a 60-80 Hz low cut tightens a downtuned guitar without losing its weight. A 1x15 or 8x10 IR in one slot and a 4x12 in the other makes a good bass blend.",
                         "<b>7 cordes en G et basse</b> : laissez le coupe-bas sur Off ou 40 Hz pour garder le grave ; à 60-80 Hz il resserre une guitare accordée bas sans lui retirer son poids. Une IR 1x15 ou 8x10 dans un slot et une 4x12 dans l'autre font un bon mélange basse."),
                       t("<b>Compare IRs at the same level</b>: keep Normalize IRs on, map IR A next / previous to two footswitches and browse while playing.",
                         "<b>Comparer des IR au même niveau</b> : gardez Normalize IRs activé, assignez IR A suivante / précédente à deux footswitchs et parcourez en jouant."),
                       t("<b>OVERLOAD</b> works best in small amounts on clean and crunch tones; on high-gain it adds a speaker breakup feel.",
                         "<b>OVERLOAD</b> s'utilise de préférence par petites touches sur sons clairs et crunch ; en high-gain il ajoute une sensation de haut-parleur qui sature.")])]
    # 10. Specs
    b += [("keep", [("h1", t("10. Technical specifications", "10. Caractéristiques techniques")),
                    ("table", [["Format", t("VST3, Windows 10/11 (64-bit) and macOS 10.13 or later (universal, AU too); MIDI input", "VST3, Windows 10/11 (64 bits) et macOS 10.13 ou plus récent (universel, AU aussi) ; entrée MIDI")],
                               [t("Inputs / outputs", "Entrées / sorties"), t("Mono &gt; mono, mono &gt; stereo, stereo &gt; stereo; true stereo with stereo IRs", "Mono &gt; mono, mono &gt; stéréo, stéréo &gt; stéréo ; stéréo vraie avec les IR stéréo")],
                               [t("Latency", "Latence"), t("2 samples (reported to the host)", "2 échantillons (déclarés à l'hôte)")],
                               [t("Convolution", "Convolution"), t("JUCE partitioned convolution, zero latency, 4096-sample head, internal crossfade on IR change", "Convolution partitionnée JUCE, sans latence, tête de 4096 points, fondu interne au changement d'IR")],
                               ["IR", t("WAV / AIFF / FLAC, mono or stereo, up to 10 s; windowed-sinc resampling (Kaiser, 24 zero crossings) to the host sample rate; trailing silence removed", "WAV / AIFF / FLAC, mono ou stéréo, jusqu'à 10 s ; rééchantillonnage sinc fenêtré (Kaiser, 24 passages par zéro) à la fréquence de l'hôte ; silence de fin retiré")],
                               [t("Level", "Niveau"), t("Unit-energy normalization (TONE3000), -6 dB for cabinet IRs; off = NAM law (-18 dB)", "Normalisation à énergie unité (TONE3000), -6 dB pour les IR de baffle ; désactivée = loi NAM (-18 dB)")],
                               [t("Parameters", "Paramètres"), "Power, Mix A/B, Overload, Variphi, Level A/B, Mute A/B, Phase A/B, Low cut, High cut, Normalize IRs " + t("(all automatable)", "(tous automatisables)")],
                               ["MIDI", t("MIDI Learn on 16 controls (CC or note), no CC by default, latching or momentary footswitch, 14-bit CC, learned pedal travel", "MIDI Learn sur 16 commandes (CC ou note), aucun CC par défaut, footswitch à accrochage ou momentané, CC 14 bits, course de pédale apprise")],
                               ["Presets", t("User presets (.impulse) with embedded IRs, export / import to any folder", "Presets utilisateur (.impulse) avec IR intégrées, export / import vers n'importe quel dossier")],
                               [t("Window", "Fenêtre"), t("60 % / 75 % / 100 %, default size of new windows selectable, never smaller than 60 %; accent colour selectable", "60 % / 75 % / 100 %, taille par défaut des nouvelles fenêtres au choix, jamais moins de 60 % ; couleur d'accent au choix")],
                               [t("Stability", "Stabilité"), t("Windows anti-freeze v2: software renderer, background cache rebuilt by the timer, redraw watchdog, no host call from the audio thread", "Anti-gel v2 sous Windows : rendu logiciel, fond refait par le timer, chien de garde du redessin, aucun appel à l'hôte depuis le thread audio")],
                               [t("Credits", "Crédits"), t("Engine based on the TONE3000 plugin and NAM / AudioDSPTools (MIT); Inter font (SIL OFL)", "Moteur d'après le plugin TONE3000 et NAM / AudioDSPTools (MIT) ; police Inter (SIL OFL)")],
                               ["Version", VERSION]],
                     [34, None], {"header": False})])]
    return b


L.build(OUT, "Impulse", VERSION, "#2AA4DC", "impulse_logo_blanc.png", TEXTS, content)
