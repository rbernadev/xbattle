// =====================================================================
// XBATTLE - Calibro di tolleranza (stampalo PRIMA di qualsiasi case)
// Piastra con fori quadrati di larghezza crescente + un perno quadrato.
// Il perno e' sempre "nominal". Il foro piu' piccolo in cui il perno
// scorre con attrito leggero = il gioco che la TUA stampante richiede.
// Il valore inciso accanto al foro e' il gioco da sommare alle quote
// nominali nei case (es. "+0.2" => aggiungi 0.2 mm per lato di accoppiamento).
// Stampa con le STESSE impostazioni (materiale, layer, orientamento) del case.
// (File non ancora renderizzato in OpenSCAD: se da errore dimmelo.)
// =====================================================================
$fn = 32;

nominal = 10;     // lato del perno (mm)
step    = 0.1;    // incremento del gioco tra un foro e il successivo
n       = 7;      // numero di fori (gioco 0.0 ... (n-1)*step)
plate_t = 6;      // spessore piastra
wall    = 4;      // parete tra i fori
label_h = 0.6;    // profondita' incisione

hole_pitch = nominal + (n-1)*step + wall;

module plate() {
    difference() {
        cube([n*hole_pitch + wall, nominal + 2*wall + 8, plate_t]);
        for (i = [0 : n-1]) {
            w = nominal + i*step;
            // foro quadrato passante
            translate([wall + i*hole_pitch, wall + 8, -1])
                cube([w, w, plate_t + 2]);
            // etichetta incisa sopra il foro (lato Y basso)
            translate([wall + i*hole_pitch + 0.5, 2, plate_t - label_h])
                linear_extrude(label_h + 1)
                    text(str("+", i*step), size = 3.2);
        }
    }
}

module peg() {
    cube([nominal, nominal, plate_t + 14]);
}

plate();
translate([0, nominal + 2*wall + 8 + 10, 0]) peg();
