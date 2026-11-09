# Bevel exterieur d'Impulse = rendu du bevel d'Ampire (memes proportions 2,5:1, meme rayon d'angle) : copie des pixels d'Ampire
# sur la bande du cadre (distance au bord < 26 px du repere 3600), fondu sur 26..32 px, alpha d'Impulse conserve.
import numpy as np, sys
from PIL import Image
from scipy.ndimage import distance_transform_edt
imp = np.asarray(Image.open(sys.argv[1]).convert('RGBA')).astype(float)
amp = np.asarray(Image.open('/home/claude/ampire/assets/bg.png').convert('RGBA').resize((3600, 1440), Image.LANCZOS)).astype(float)
a = imp[..., 3] > 127
d = distance_transform_edt(np.pad(a, 1))[1:-1, 1:-1]
k = np.clip((32.0 - d) / 6.0, 0, 1)[..., None]
out = imp.copy()
# couleur d'Ampire (non premultipliee) ; la ou Ampire est transparent (angle a peine plus grand), on garde Impulse
aa = (amp[..., 3:4] / 255.0)
src = amp[..., :3] * aa + imp[..., :3] * (1 - aa)
out[..., :3] = imp[..., :3] * (1 - k) + src * k
Image.fromarray(np.clip(out, 0, 255).astype(np.uint8), 'RGBA').save(sys.argv[2])
