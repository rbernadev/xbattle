# Misure da prendere sui lanciatori (String e Winder)

## Cosa sappiamo già
- Tutti i lanciatori (String, Winder, e il Launcher Grip) hanno la **stessa "Battle Pass Port"**: 4 incavi per le sporgenze del Battle Pass
  (manuale BX-09, vedi `ORIGINALE_BX09.md`) e, a giudicare dal riferimento Thingiverse, due tacche laterali per i ganci (da confermare sul tuo pezzo).
- La community ha già decodificato i ganci: `hardware/case/reference/thingiverse_6958498` (piastra ~35 x 34 mm, ganci 13,75 x 11,91 x 14).
- Takara Tomy non pubblica le quote ufficiali: ciò che manca va misurato sui **tuoi** lanciatori.

## Passo 0 — Coupon di prova (zero rischi)
Stampa dal riferimento Thingiverse **solo la piastra e i 2 ganci** (non la parte con la livella a bolla) e provali su String e Winder.
Se si agganciano, la base meccanica è validata e possiamo riusarla. Stampa prima `hardware/case/tools/tolerance_gauge.scad`
con le stesse impostazioni, così sai il gioco della tua stampante.

## A. Foto (con calibro o righello affiancato per la scala), per ciascun lanciatore
1. **Battle Pass Port dall'alto**, perpendicolare, a piena risoluzione (è la foto più importante).
2. Port in diagonale (si devono vedere profondità e bordi dei 4 incavi).
3. Primo piano degli **incavi** e delle **sporgenze laterali** dove si agganciano i ganci.
4. **Finestra/foro** nella port (se c'è) e **cosa si vede sotto**: ingranaggio, cremagliera, cordino, riflettente? Torcia puntata dentro.
5. Lanciatore di lato e di punta (quote d'ingombro).

## B. Quote (mm, con calibro)
| # | Quota | String | Winder |
|---|-------|--------|--------|
| 1 | Port: lunghezza x larghezza x profondità della zona incassata | | |
| 2 | I 4 incavi: posizione (X,Y dallo spigolo della port), larghezza, lunghezza, profondità | | |
| 3 | Sporgenze/tacche per i ganci: posizione, larghezza, altezza dalla superficie | | |
| 4 | Finestra del sensore (se esiste): posizione (X,Y), forma, dimensioni | | |
| 5 | Distanza tra la superficie della port e la parte mobile visibile sotto la finestra | | |
| 6 | Parte mobile: tipo (ingranaggio / cremagliera / cordino), n. denti, passo, gira o scorre? | | |
| 7 | Dove NON deve esserci ingombro (mano, cordino, uscita del bey, tasto di blocco) | | |
| 8 | Con il Launcher Grip montato: dove resta spazio per il Battle Pass? | | |

## C. Domande che cambiano il progetto
- Il sensore dell'originale legge una parte del **lanciatore** (non del bey)? String e Winder hanno rapporti diversi? In questo caso il firmware deve avere una costante per lanciatore.
- Hai in casa un Battle Pass originale (anche di un amico)? Il confronto con i suoi valori "SP" permetterebbe di calibrare.
- Resina o FDM? La resina standard rompe i ganci sottili: serve resina tough, oppure FDM (PETG) per i ganci e resina per il guscio.

## D. Flusso consigliato
1. Calibro di tolleranza → coupon piastra+ganci → verifica il fit su entrambi i lanciatori.
2. Foto A + tabella B.
3. Coupon della port (solo l'attacco + finestra sensore), si itera finché calza e il TCRT5000 legge.
4. Solo dopo, il guscio con l'elettronica.
