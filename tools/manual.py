# IMPULSE (NINE HANDS | DSP) - notice PDF en francais (couverture « minimale »).
# Usage : python3 tools/manual.py [fichier PDF de sortie]
# Version lue dans CMakeLists.txt. Illustrations : tools/manual_assets/ (rendus reels de l'interface, outil tools/ImpulseTool).
import os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import manual_lib as L

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
VERSION = re.search(r"project\s*\(\s*\w+\s+VERSION\s+([0-9.]+)", open(os.path.join(ROOT, "CMakeLists.txt")).read()).group(1)
OUT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(ROOT, "docs", "Impulse_Notice_FR_v" + VERSION + ".pdf")


def content():
    b = []
    # 1. Presentation
    b += [("h1", "1. Présentation"),
          ("p", "<b>Impulse</b> charge des <b>réponses impulsionnelles</b> (IR) : de courts fichiers audio qui capturent la façon dont un baffle et son micro colorent le son. Placé après un simulateur d'ampli (baffle désactivé), une capture NAM ou une pédale de préampli, il transforme le signal brut de l'ampli en baffle repris au micro, pour guitare et basse."),
          ("p", "Deux emplacements, <b>A</b> et <b>B</b>, se chargent en même temps et se mélangent, comme deux micros sur le même baffle : MIX les dose, VARIPHI décale la phase de B, et chaque emplacement a son niveau, sa coupure et son inversion de phase."),
          ("p", "Le moteur audio suit le plugin open source <b>TONE3000</b> et <b>Neural Amp Modeler (NAM)</b> (licence MIT tous les deux) : convolution partitionnée sans latence, égalisation des niveaux à énergie unité et loi de gain à -18 dB. Impulse va plus loin sur le rééchantillonnage : une IR enregistrée à une autre fréquence est convertie par un rééchantillonneur sinc fenêtré (Kaiser), sa réponse en fréquence reste à moins de 0,1 dB de l'originale."),
          ("p", "Impulse <b>accepte le MIDI</b> : chaque commande s'apprend par clic droit (section 6). Aucun CC n'est assigné par défaut.")]
    # 2. Installation
    b += [("h1", "2. Installation"),
          ("h2", "Windows"),
          ("p", "Téléchargez l'artefact <b>Impulse-VST3-Windows</b> (GitHub Actions, dépôt Nine-Hands-Dsp/Impulse), décompressez-le et copiez <b>Impulse.vst3</b> dans <b>C:\\Program Files\\Common Files\\VST3\\</b>."),
          ("h2", "macOS"),
          ("p", "Téléchargez <b>Impulse-VST3-macOS</b> (universel : Apple Silicon et Intel, macOS 10.13+), décompressez-le (il contient un autre zip), décompressez encore et copiez <b>Impulse.vst3</b> dans <b>~/Library/Audio/Plug-Ins/VST3/</b>. Le plugin n'est pas notarié : lancez une fois dans le Terminal <b>xattr -dr com.apple.quarantine ~/Library/Audio/Plug-Ins/VST3/Impulse.vst3</b>"),
          ("h2", "Reaper"),
          ("p", "Options &gt; Preferences &gt; Plug-ins &gt; VST &gt; <b>Re-scan</b>. Impulse apparaît sous le nom <b>Impulse (NINE HANDS | DSP)</b>. Pour le MIDI, la piste doit envoyer le MIDI au plugin : armez-la avec votre contrôleur MIDI en entrée, ou utilisez un envoi MIDI."),
          ("pb",)]
    # 3. Overview
    rows = [["#", "Commande", "Rôle"],
            ["1", "Power", "Effet actif / coupé (fondu doux avec le son direct)"],
            ["2", "Menu des presets", "Flèches : preset précédent / suivant ; clic sur le nom : liste et options"],
            ["3", "Fichier preset", "Save, Save as, export vers un fichier, import depuis un fichier, dossier des presets"],
            ["4", "Menu", "Plugin size, Settings, MIDI (le menu du clic droit sans Presets)"],
            ["5", "Nom de l'IR", "Clic : charger un fichier ; déposez un fichier ici ; clic droit : menu du slot"],
            ["6", "Bibliothèque", "Bouton liste + loupe : ouvre la bibliothèque d'IR"],
            ["7", "Précédente / suivante", "Charge l'IR précédente / suivante du même dossier"],
            ["8", "Forme d'onde", "L'IR telle qu'elle est utilisée, avec durée, fréquence, résolution, mono / stéréo"],
            ["9", "M", "Coupe le slot"],
            ["10", "\u00d8", "Inverse la polarité du slot"],
            ["11", "Fader de niveau", "+12 dB à -\u221e, 0 dB par défaut"],
            ["12", "MIX", "Dosage entre l'IR A et l'IR B"],
            ["13", "OVERLOAD", "Surcharge du haut-parleur (saturation avant les IR)"],
            ["14", "VARIPHI", "Retard fin de B (0 à 1 ms) : phase entre les deux micros"],
            ["15", "Ligne d'état", "Valeur de la commande qui bouge, MIDI Learn, messages"]]
    b += [("h1", "3. Vue d'ensemble de l'interface"),
          ("img", "annotated.png", 168, "Le slot B (à droite) fonctionne exactement comme le slot A."),
          ("table", rows, [8, 34, None], {"small": True})]
    # 4. Controls
    b += [("cond", 60), ("h1", "4. Les commandes"),
          ("h2", "Charger des IR (slots A et B)"),
          ("bullets", ["<b>Formats</b> : WAV, AIFF, FLAC, 16 / 24 / 32 bits, toute fréquence, mono ou stéréo (un fichier à plus de canaux utilise ses deux premiers). Jusqu'à 10 secondes : IR de baffle, mais aussi de pièce ou de reverb.",
                       "<b>Charger</b> : cliquez sur le cadre du nom ou sur le bouton liste + loupe pour ouvrir la <b>bibliothèque d'IR</b> (ci-dessous), ou <b>glissez-déposez</b> un fichier sur un slot. Déposez <b>deux fichiers d'un coup</b> : le premier va dans A, le second dans B. Déposer un dossier charge sa première IR.",
                       "<b>Dossier des IR par défaut</b> : clic droit ou roue dentée &gt; Settings &gt; IR folder &gt; Choose default IR folder... indique où se trouve votre bibliothèque d'IR (section 5). Quand un slot contient déjà une IR, le bouton parcourir et les flèches utilisent le dossier de cette IR.",
                       "<b>Parcourir</b> : les flèches chargent l'IR précédente / suivante du même dossier (ordre naturel, en boucle). Le changement se fait en fondu, sans clic : idéal pour comparer des micros en jouant.",
                       "<b>Clic droit</b> sur un nom ou une forme d'onde : Open IR library, Load IR file (sélecteur de fichiers), IR précédente / suivante du dossier, Show in folder, Clear, et le gain appliqué à cette IR.",
                       "Un fichier illisible affiche un message rouge quelques secondes ; l'IR déjà chargée reste en place."]),
          ("h2", "Bibliothèque d'IR"),
          ("img", "library.png", 150, "Onglet Library : dossiers, sous-dossiers et IR de votre dossier d'IR."),
          ("bullets", ["Une fenêtre en verre fumé s'ouvre au-dessus d'Impulse (même bibliothèque que celle des captures d'Ampire). Elle charge l'IR choisie dans le slot sur lequel vous avez cliqué ; la croix, Échap ou un clic sur l'IR la referment.",
                       "<b>Onglet Library</b> : trois colonnes (dossiers, sous-dossiers, IR) de votre dossier d'IR par défaut, sous-dossiers compris. <b>Favourites</b> (étoile devant chaque IR) et <b>Recent</b> (dernières IR chargées) en tête de la première colonne. Chaque IR porte la pastille <b>IR</b> et l'indication Mono ou Stereo ; celle du slot est surlignée.",
                       "<b>Recherche</b> : tapez quelques lettres (nom, dossier) ; Entrée charge le premier résultat. Le bouton dossier change le dossier de la bibliothèque, le bouton rond relit le dossier (Rescan). Le dossier est relu en arrière-plan et gardé en cache : des milliers d'IR s'ouvrent aussitôt.",
                       "<b>Onglet TONE3000</b> : connectez votre compte TONE3000 (le navigateur s'ouvre, connexion par e-mail et code à 6 chiffres) pour voir vos favoris, vos tones synchronisés ou créés, et les tendances. Seuls les tones qui contiennent des <b>IR</b> sont listés ; <b>SYNC</b> télécharge leurs IR dans votre dossier d'IR, rangées comme NAMMAN (Cabinets_IRs / nom du tone), et <b>Re-sync</b> relit ce dossier ou met à jour les tones déjà synchronisés.",
                       "<b>Défilement</b> : à la molette, la bibliothèque s'agrandit vers le bas et déborde sous Impulse (à la taille d'Impulse : 60, 75 ou 100 %) ; l'en-tête se réduit à une ligne pendant le défilement puis revient dès que la molette s'arrête. Elle reste agrandie jusqu'à la croix (ou Échap). Chaque colonne défile seule : arrivée en bas, elle y reste. Les barres de défilement s'attrapent aussi à la souris.",
                       "Clic droit sur une IR ou un dossier : charger, favori, montrer dans le dossier, renommer, supprimer (mis à la corbeille)."]),
          ("h2", "Niveau, M et \u00d8"),
          ("p", "Chaque slot a un fader de <b>+12 dB à -\u221e</b> (double-clic sur le curseur : retour au gain unité, 0 dB ; Maj + glisser : réglage fin), un bouton <b>M</b> qui le coupe (sa forme d'onde s'assombrit) et un bouton <b>\u00d8</b> qui inverse sa polarité, utile quand deux micros s'annulent."),
          ("h2", "MIX"),
          ("p", "Le potard affiche la quantité et le côté qui domine : <b>50A</b> = à mi-course vers A (A 75 %, B 25 %), <b>30B</b> = vers B, <b>0</b> = centre. Une petite <b>butée</b> retient le potard sur 0 quand on passe d'un côté à l'autre (à la molette aussi) ; un double-clic n'importe où sur le potard, valeur comprise, le remet au centre."),
          ("p", "De <b>100A</b> (A seul) à <b>100B</b> (B seul), <b>0</b> = les deux à -6 dB chacun, un fondu linéaire qui garde le niveau constant pour deux IR du même baffle. Quand un seul slot est actif (l'autre vide ou coupé), il joue à 100 % quel que soit MIX. Sans aucune IR, le son passe sans traitement."),
          ("h2", "OVERLOAD"),
          ("p", "Simule un haut-parleur poussé au-delà de ses limites, avant les IR : saturation asymétrique (harmoniques paires) jusqu'à +24 dB d'attaque avec compensation partielle du niveau. La saturation est anti-repliement (ADAA) sans suréchantillonnage, donc sans latence, et à <b>0 %</b> le signal est strictement inchangé. Un peu (10 à 30 %) ajoute compression et épaisseur ; plus haut, le baffle se met à saturer."),
          ("h2", "VARIPHI"),
          ("p", "Retarde l'IR B de <b>0 à 1 ms</b> (100 % = 1 ms, environ 34 cm d'écart de micro), avec un retard fractionnaire sans saccade. Entre deux micros, cela déplace le filtrage en peigne : 0,5 ms met un creux à 1 kHz. Tournez-le lentement avec les deux IR à MIX 0 pour trouver le mélange le plus plein ou le plus creux. Avec A seul, VARIPHI n'a aucun effet."),
          ("h2", "Potards"),
          ("p", "Glisser vers le haut / bas (Maj : fin), molette, double-clic : valeur par défaut. La valeur s'affiche dans le potard et dans la ligne d'état.")]
    # 5. Settings
    b += [("h1", "5. Menu (clic droit ou roue dentée)"),
          ("p", "Un clic droit n'importe où ouvre le même menu que sur les autres plugins NINE HANDS | DSP ; la roue dentée l'ouvre aussi, sans le sous-menu Presets. Sur une commande, son <b>MIDI Learn</b> et <b>Clear MIDI</b> viennent en premier ; sur un slot (nom ou forme d'onde), les actions de l'IR viennent en premier (Load IR, IR précédente / suivante du dossier, Show in folder, Clear, gain appliqué). Ensuite : <b>Plugin size</b>, <b>Presets</b>, <b>Settings</b> et <b>MIDI</b>. En haut du menu, un en-tête rappelle le nom et la version du plugin (même présentation que les menus d'Octopus). Le sous-menu <b>Settings</b> contient :"),
          ("table", [["Élément", "Rôle"],
                     ["Colour...", "Nuancier (section 7)"],
                     ["IR folder", "Dossier des IR par défaut (et de la bibliothèque), commun à toutes les fenêtres d'Impulse : le bouton parcourir d'un slot vide s'ouvre dedans, et les flèches d'un slot vide le parcourent. Choose default IR folder..., Show default IR folder, ou No default folder (dernier dossier utilisé)."],
                     ["Normalize IRs", "Activé (par défaut) : chaque IR est ramenée au même niveau (énergie unité, méthode TONE3000, corrigée de la fréquence d'échantillonnage), -6 dB pour une IR de baffle pour que la sortie soit à peu près au niveau de l'entrée sur un signal d'ampli, sans pad pour une IR longue (reverb, &gt; 1 s). Désactivé : IR brute à -18 dB (loi NAM)."],
                     ["Low cut", "Off, 40, 60, 80, 100, 120, 150 Hz (12 dB/oct, après le mélange)"],
                     ["High cut", "Off, 12, 10, 8, 7, 6, 5 kHz (12 dB/oct)"],
                     ["MIDI footswitches", "À accrochage (127 = on / 0 = off, par défaut) ou momentané (chaque appui inverse), pour Power, Mute et Phase (section 6)"]],
           [32, None]),
          ("p", "Coupe-bas et coupe-haut sont aussi des paramètres automatisables à plage continue (20 à 400 Hz, 2 à 20 kHz). Le silence de fin d'une IR est retiré automatiquement, mais son début est gardé pour que deux IR enregistrées ensemble restent alignées en phase.")]
    # 6. MIDI
    b += [("cond", 90), ("h1", "6. MIDI et pédales d'expression"),
          ("p", "<b>Clic droit sur une commande</b> &gt; MIDI Learn, puis bougez un potard / une pédale ou appuyez sur un footswitch de votre contrôleur : la ligne d'état confirme « Mix A/B = CC 11 ». Clic droit &gt; Clear MIDI l'efface. Un numéro de CC déjà pris passe à la nouvelle commande. <b>Aucun CC n'est assigné par défaut</b> ; les assignations sont sauvegardées avec le projet."),
          ("table", [["Apprenables", "Comportement"],
                     ["Mix A/B, Overload, Variphi, Level A, Level B",
                      "Continu. Course de la pédale d'expression apprise automatiquement (une pédale qui n'envoie que 2-116 atteint quand même 0 et 100 %), petites zones mortes aux butées, CC 14 bits accepté, lissé sans bruit d'escalier."],
                     ["Mute A, Mute B, Phase A, Phase B, Power",
                      "Footswitch à accrochage : 127 = on, 0 = off. Avec Settings &gt; « MIDI footswitches momentary (toggle each press) » chaque appui inverse. Peut aussi apprendre une note MIDI."],
                     ["IR A précédente / suivante, IR B précédente / suivante, preset précédent / suivant",
                      "Un pas par appui (CC &ge; 64 ou note) : parcourez baffles ou presets au pied."]],
           [52, None]),
          ("p", "Un clic droit sur une zone vide (ou roue dentée &gt; MIDI) donne la liste complète avec les assignations en cours.")]
    # 7. Window, colour
    b += [("h1", "7. Fenêtre et couleur"),
          ("p", "La fenêtre s'ouvre à la taille par défaut, appelée <b>60 %</b>, et ne peut jamais être plus petite. Clic droit &gt; Plugin size : <b>60 % / 75 % / 100 %</b> pour cette fenêtre, et <b>Default size (new windows)</b> pour toutes les nouvelles fenêtres d'Impulse. La poignée du coin l'agrandit encore, proportions conservées. La taille de chaque instance est sauvegardée avec le projet."),
          ("h2", "Empilé sous Ampire (REAPER)"),
          ("p", "Dans REAPER, quand Ampire et Impulse sont sur la même piste, l'inverseur <b>STACK</b> d'Ampire (à gauche de POWER), levier en haut, affiche cet Impulse collé sous Ampire, dans la même fenêtre. Impulse reste à sa place dans la chaîne d'effets et garde sa propre fenêtre ; les deux vues pilotent le même Impulse. Levier en bas, il est masqué ; le choix est sauvegardé avec le projet. S'il y a plusieurs Impulse sur la piste, Ampire affiche le premier. Dans cette vue, le menu des presets reste dans la zone d'Impulse (il défile s'il est long)."),
          ("side", "picker.png", 62,
           [("h2", "Couleur d'accent"),
            ("p", "Clic droit ou roue dentée &gt; Settings &gt; <b>Colour...</b> ouvre le nuancier d'Octopus : couleurs rapides, <b>MY COLOURS</b> (clic sur un emplacement vide pour enregistrer la couleur, clic droit pour l'effacer), teinte / saturation / luminosité, et une case pour <b>taper un code couleur</b> (ex. #172633). La couleur change en direct ; DEFAULT rend le bleu d'origine, OK ferme."),
            ("p", "Un code tapé dans la case s'applique avec Entrée ou OK. La couleur est sauvegardée avec le projet et devient celle des nouvelles fenêtres d'Impulse. Une couleur très sombre rend les formes d'onde peu visibles.")]),
          ("img", "colour.png", 92, "La même interface avec un accent orange et une IR stéréo dans le slot B.")]
    # 8. Presets
    b += [("cond", 70), ("h1", "8. Presets"),
          ("p", "Impulse n'a pas de presets d'usine (les IR sont vos propres fichiers). Le menu du haut (et clic droit &gt; Presets) contient <b>Init (empty)</b>, vos presets par ordre alphabétique, <b>Save</b>, <b>Save preset as...</b>, <b>Delete</b>, <b>Export preset to file...</b>, <b>Import preset from file...</b> et <b>Open presets folder</b> ; la <b>disquette</b> à côté de la roue dentée donne les mêmes actions de fichier. Les flèches à gauche du menu chargent le preset précédent / suivant. Une <b>*</b> après le nom signale un réglage modifié depuis le chargement ; « (Custom) » signifie qu'aucun preset n'est chargé."),
          ("p", "Un preset garde tous les paramètres et les deux IR : le chemin du fichier, plus une <b>copie de l'IR</b> (jusqu'à 4 s) dans le preset, qui sonne donc encore juste si le fichier a été déplacé ou renommé (la ligne d'infos affiche alors « saved copy »). Les projets gardent la même copie. Les assignations MIDI restent liées à l'instance, pas au preset."),
          ("p", "<b>Export</b> écrit le réglage en cours, IR comprises, dans un fichier .impulse où vous voulez (sauvegarde, autre ordinateur). <b>Import</b> charge un fichier .impulse et le copie dans votre liste de presets ; si le nom existe déjà, un numéro est ajouté au lieu d'écraser."),
          ("cond", 22), ("table", [["", "Dossier des presets"],
                     ["Windows", "%APPDATA%\\NINE HANDS DSP\\Impulse\\Presets\\*.impulse"],
                     ["macOS", "~/Library/Application Support/NINE HANDS DSP/Impulse/Presets/*.impulse"]],
           [24, None])]
    # 9. Tips
    b += [("h1", "9. Conseils d'utilisation"),
          ("bullets", ["<b>Chaîne dans Reaper</b> : guitare &gt; vos pédales NINE HANDS | DSP &gt; ampli (Ampire avec une capture NAM, Anode, préampli) &gt; <b>Impulse</b> &gt; reverb / delay. Un chargeur d'IR se place toujours après l'ampli.",
                       "<b>Deux micros</b> : chargez le SM57 et le ruban (ou statique) du même pack dans A et B, MIX à 0, puis tournez VARIPHI lentement ; si le mélange sonne maigre, essayez \u00d8 sur B.",
                       "<b>7 cordes en G et basse</b> : laissez le coupe-bas sur Off ou 40 Hz pour garder le grave ; à 60-80 Hz il resserre une guitare accordée bas sans lui retirer son poids. Une IR 1x15 ou 8x10 dans un slot et une 4x12 dans l'autre font un bon mélange basse.",
                       "<b>Comparer des IR au même niveau</b> : gardez Normalize IRs activé, assignez IR A suivante / précédente à deux footswitchs et parcourez en jouant.",
                       "<b>OVERLOAD</b> s'utilise de préférence par petites touches sur sons clairs et crunch ; en high-gain il ajoute une sensation de haut-parleur qui sature."])]
    # 10. Specs
    b += [("keep", [("h1", "10. Caractéristiques techniques"),
                    ("table", [["Format", "VST3, Windows 10/11 (64 bits) et macOS 10.13 ou plus récent (universel, AU aussi) ; entrée MIDI"],
                               ["Entrées / sorties", "Mono &gt; mono, mono &gt; stéréo, stéréo &gt; stéréo ; stéréo vraie avec les IR stéréo"],
                               ["Latence", "2 échantillons (déclarés à l'hôte)"],
                               ["Convolution", "Convolution partitionnée JUCE, sans latence, tête de 4096 points, fondu interne au changement d'IR"],
                               ["IR", "WAV / AIFF / FLAC, mono ou stéréo, jusqu'à 10 s ; rééchantillonnage sinc fenêtré (Kaiser, 24 passages par zéro) à la fréquence de l'hôte ; silence de fin retiré"],
                               ["Niveau", "Normalisation à énergie unité (TONE3000), -6 dB pour les IR de baffle ; désactivée = loi NAM (-18 dB)"],
                               ["Paramètres", "Power, Mix A/B, Overload, Variphi, Level A/B, Mute A/B, Phase A/B, Low cut, High cut, Normalize IRs " + "(tous automatisables)"],
                               ["MIDI", "MIDI Learn sur 16 commandes (CC ou note), aucun CC par défaut, footswitch à accrochage ou momentané, CC 14 bits, course de pédale apprise"],
                               ["Presets", "Presets utilisateur (.impulse) avec IR intégrées, export / import vers n'importe quel dossier"],
                               ["Fenêtre", "60 % / 75 % / 100 %, taille par défaut des nouvelles fenêtres au choix, jamais moins de 60 % ; couleur d'accent au choix"],
                               ["Stabilité", "Anti-gel v2 sous Windows : rendu logiciel, fond refait par le timer, chien de garde du redessin, aucun appel à l'hôte depuis le thread audio"],
                               ["Crédits", "Moteur d'après le plugin TONE3000 et NAM / AudioDSPTools (MIT) ; police Inter (SIL OFL)"],
                               ["Version", VERSION]],
                     [34, None], {"header": False})])]
    return b


L.build(OUT, "Impulse", VERSION, "#2AA4DC", "#8FD3F2", "IMPULSE", "Chargeur d'IR double", content)
