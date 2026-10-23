# Beyblade X BX-09 BeyBattle Pass — com'è fatto e come si usa (dal manuale ufficiale)

Fonte: manuale d'uso Takara Tomy BX-09 (16 pagine) + "WEB遊び方説明書" (27 pagine), forniti da Riccardo.
I PDF non sono nel repo (copyright): tienili in `docs/reference/` (cartella ignorata da git).
Traduzione dal giapponese a cura di Claude: dove il testo era ambiguo è segnalato.

## Struttura (manuale p.4)
- **Sopra:** cover trasparente, "Battle Pass Sheet" (adesivo sotto la cover), **LED** (un solo LED multicolore), **pulsante**, **2 lock parts** (ganci bianchi laterali).
- **Sotto:** coperchio batterie (vite a croce), **reset switch**, **interruttore di riconoscimento bey** (ベイブレード装着認識スイッチ) e **sensore IR**, vicini tra loro.
- **Alimentazione:** 2 pile AAA alcaline (non ricaricabili). **Non c'è interruttore**: si accende col pulsante (1 s) e si spegne da solo dopo 5 minuti di inattività o col reset. I dati registrati non si perdono allo spegnimento.
- **Comunicazione:** Bluetooth 2.4 GHz con l'app "BEYBLADE X". Non impermeabile.

## Attacco al lanciatore (p.6-8)
1. Le **4 sporgenze (凸)** sul fondo del Battle Pass si incastrano nei **4 incavi (凹)** del lanciatore. Entra in **un solo verso**; forzarlo al contrario può rompere il pezzo.
2. Poi si premono dall'alto **entrambi i lock parts** nel verso delle frecce, finché fanno **click**.
3. Se i lock parts non sono fissati bene, "la potenza di lancio può non essere misurata correttamente".
4. Smontaggio: si spinge un lock part dal basso verso l'alto e si inclina il Battle Pass.
5. Stessa interfaccia su **Winder Launcher** e **String Launcher**. Con un **Launcher Grip**: prima si monta il grip, poi il Battle Pass (al contrario non si riesce).
6. Per sparare si impugna **il lanciatore**, mai il Battle Pass.

> Questo corregge la descrizione "guida a T con dente a molla" usata nelle note precedenti: dal manuale
> l'attacco è **4 sporgenze/incavi + 2 ganci laterali che scattano** (vedi anche `hardware/case/reference`).

## Uso e stati del LED (p.11-12)
| Azione | LED |
|---|---|
| Pulsante premuto ~1 s | **verde** fisso = acceso, in attesa |
| Bey inserito nel lanciatore | **arancione** fisso = pronto a misurare |
| Lancio misurato | **verde, 4 lampeggi**; si registrano potenza e conteggio |
| Nuovo record | **verde → arancione → rosso** veloci |
| Misura non riuscita / errore | **rosso, 4 lampeggi** |

## Dati registrati (p.10)
- **MAX shoot power**, **conteggio cumulativo dei lanci**, **ultimi 50 shoot power** (dal 51° sovrascrive i più vecchi).
- Il MAX si azzera dopo la comunicazione con l'app.
- Valori d'esempio nel manuale: 3210, 6480, 5000, 4300, 5500; nell'app: 7583 (MAX), 166 lanci, 4863, 373, 779; "SP 7075".
  **Scala dell'ordine di migliaia, non confrontabile con gli RPM**: la relazione SP↔RPM non è nota.

## Comunicazione con l'app (p.13-25)
- Prima volta: scansione del **BEYCODE** stampato sulla confezione (il flap), poi "registrazione Battle Pass".
- Per collegarsi: pulsante ~1 s → LED **verde ↔ arancione alternati** = pronto a comunicare. Il **bey va tolto dal lanciatore**.
- Collegamento riuscito: verde 4 lampeggi. Fallito: rosso 4 lampeggi e ritorno in attesa.
- Per terminare: dall'app, oppure **tenere premuto 3 s** (rosso 4 lampeggi).
- Funzioni app: lettura dati (MAX, conteggio, ultimi 50) e "Shoot Gym" (battaglia contro il computer con SP in tempo reale).

## Risoluzione problemi (p.27) — utile per il nostro progetto
- Misura che non funziona: attacco non corretto, **sensore IR sul retro sporco** (pulire con panno asciutto), **evitare luce solare forte e lampade a incandescenza** (interferiscono col sensore).
- Il sensore è quindi ottico/IR e sensibile alla luce ambiente → nel nostro case il sensore deve essere **schermato dalla luce esterna**.

## Cosa NON sappiamo ancora
- **Cosa legge esattamente il sensore IR** dell'originale (quale parte mobile del lanciatore, attraverso quale finestra).
- Le quote reali dell'attacco (posizione/dimensioni di sporgenze e incavi) e della finestra del sensore.
- Come l'originale converte ciò che legge in "shoot power".
Per chiarirlo servono le foto/misure di `docs/MISURE_LANCIATORI.md`.

## Cosa copiamo per l'esperienza d'uso
Stati e feedback (acceso / pronto / ok / record / errore), pulsante 1 s = comunicazione, 3 s = termina, spegnimento dopo
5 minuti, ultimi N lanci + MAX + contatore. Il nostro OLED mostra già i valori; i LED di fallback possono seguire la tabella sopra.
