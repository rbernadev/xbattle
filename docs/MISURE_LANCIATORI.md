# Misure da prendere sui lanciatori (String e Winder)

Takara Tomy/Hasbro **non pubblicano le quote** dei lanciatori né dell'attacco del Battle Pass originale (BX-09).
Il case v4 usava quote stimate ("26x14 mm stimato, DA VALIDARE", binari 22 mm inventati): per questo non calza.
Per un fit perfetto servono misure reali. Strumento indispensabile: **calibro digitale** (~10 €, risoluzione 0.01 mm).

> Se hai il **Battle Pass originale (BX-09)** o un **Launcher Level / Launcher Grip** originale, misura anche il
> loro attacco: è l'interfaccia ufficiale che il lanciatore accetta (sembra esserci un attacco standard,
> sopra e di lato, usato da grip, level e Battle Pass). Copiarla è molto meglio che inventarla.

## A. Foto (con un righello/calibro affiancato per la scala)
Per **ciascun** lanciatore (String, Winder), a piena risoluzione, sfondo neutro:
1. dall'alto (perpendicolare)
2. di lato (entrambi i lati)
3. dal basso
4. frontale e posteriore (di punta)
5. primo piano su qualunque guida, incastro, scanalatura o sporgenza usata per attaccare grip/level
6. primo piano sulla parte dentata (ingranaggio / cremagliera / cordino) che dovrà leggere il sensore

## B. Quote (mm, con calibro)
| # | Quota | String | Winder |
|---|-------|--------|--------|
| 1 | Lunghezza totale del corpo | | |
| 2 | Larghezza massima / spessore massimo | | |
| 3 | Larghezza e spessore dell'impugnatura nel punto dove si attacca il grip | | |
| 4 | Interfaccia di attacco (guida/incastro/foro): larghezza, profondità, lunghezza, angoli, posizione dal fronte | | |
| 5 | Dove NON deve esserci ingombro (mano, cordino, uscita del bey, tasto di blocco) | | |
| 6 | Parte che il sensore deve leggere: tipo (ingranaggio / cremagliera / cordino), n. denti, passo, posizione | | |
| 7 | Distanza tra superficie di montaggio e parte da leggere (il TCRT5000 lavora bene a ~2-5 mm) | | |
| 8 | La parte da leggere gira o scorre? A che velocità rispetto al bey (rapporto di trasmissione)? | | |

## C. Domande che cambiano il progetto
- La lettura RPM avviene sull'ingranaggio del **lanciatore** (non sul bey): String e Winder hanno denti/rapporti diversi?
  Se sì, il firmware deve avere una costante per lanciatore (oggi `PULSES_PER_REV = 6` è fisso).
- Hai già un Battle Pass originale per confrontare il valore "launch power" dell'app con i tuoi RPM?
- Resina o FDM? Con la resina standard i ganci a scatto sottili si spezzano: serve resina "tough" oppure un altro tipo di blocco.

## D. Flusso di lavoro consigliato
1. Stampa `hardware/case/tools/tolerance_gauge.scad` → trovi il gioco reale della tua stampante.
2. Compila la tabella B e manda le foto A.
3. Si fa un **coupon di prova** (solo l'attacco, ~5 min di stampa) per ciascun lanciatore, si itera.
4. Solo a fit validato si disegna il guscio con l'elettronica.
