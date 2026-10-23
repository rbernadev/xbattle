// =====================================================================
// BEYBLADE X - BATTLE PASS DIY - v4 (REVISION ESTERNA)
// Il sistema si posiziona SOPRA il lanciatore originale tramite
// slitte inferiori. Dimensionato su Ripcord e String launcher.
// =====================================================================

/* ============================ */
/*      PARAMETRI UTENTE        */
/* ============================ */

// --- Corpo principale ---
body_length          = 130;  // 13cm (come originale)
body_width           = 32;   // 3.2cm 
body_height          = 30;   // 3cm (come originale)
wall_thickness       = 2.4;
corner_radius        = 5;

// --- Finestra superiore (ispezione) ---
window_length        = 38;
window_width         = 16;
window_inset         = 44;

// --- BINARI DI AGGANCIO (T-Slot per slitta lanciatore) ---
// Regolare per il fit sul lanciatore
rail_width           = 22.0;  // Larghezza slitta (interna)
rail_height          = 8.0;   // Discesa clip (asse Z negativo)
rail_lip_depth       = 2.5;   // Rientranza del gancio
clip_thickness       = 3.0;   // Spessore alette
clip_length          = 40.0;  // Lunghezza del blocco di aggancio

// --- GANCIO DI BLOCCO MECCANICO (Bilanciere Z con molla in metallo) ---
pivot_hole_dia       = 2.0;   // Foro asse (filamento 1.75mm)
lock_w               = 6.0;   // Altezza (Z) della levetta
lock_len             = 24.0;  // Lunghezza totale (X)
lock_depth           = 4.0;   // Profondita (Y) della levetta

// --- Vano sensore TCRT5000 ---
sensor_pcb_w         = 32;
sensor_pcb_d         = 14;
sensor_pcb_h         = 7;
sensor_slot_clearance = 0.6;
sensor_sight_dia     = 6;

// --- Vano elettronica: XIAO ESP32-C3 + LiPo 802540 ---
mcu_w                = 21;
mcu_d                = 17.5;
mcu_h                = 6;
batt_w               = 25;
batt_d               = 40;
batt_h               = 8;
elec_margin          = 3;
elec_rail_depth      = 1.2;
usb_slot_w           = 10;
usb_slot_h           = 5;

// --- Sportello vano elettronica ---
lid_clearance        = 0.3;
lid_thickness        = 2.2;

// --- PULSANTE FRONTALE ---
button_hole_dia      = 9;
button_switch_w      = 6;
button_cavity_depth  = 8;

// --- DISPLAY OLED 0.91" 128x32 AZ-Delivery ---
oled_pcb_w           = 38.2;
oled_pcb_d           = 12.0;
oled_pcb_h           = 3.3;
oled_clearance       = 0.5;
oled_window_w        = 31.0;
oled_window_h        = 8.0;
oled_wire_channel_w  = 6;
oled_wire_channel_h  = 3;
oled_x_offset        = window_inset + window_length + 4;
oled_cap_lip         = 1.2;

$fn = 48;

/* ============================ */
/*      MODULI BASE             */
/* ============================ */

module rounded_box(w, d, h, r) {
    hull() {
        for (x = [r, w - r])
            for (y = [r, d - r])
                translate([x, y, 0])
                    cylinder(h = h, r = r);
    }
}

module main_shell() {
    difference() {
        rounded_box(body_length, body_width, body_height, corner_radius);

        translate([wall_thickness, wall_thickness, wall_thickness])
            rounded_box(
                body_length - 2*wall_thickness,
                body_width  - 2*wall_thickness,
                body_height - 2*wall_thickness,
                max(corner_radius - wall_thickness, 0.5)
            );

        translate([window_inset, (body_width - window_width)/2, body_height - wall_thickness - 0.5])
            cube([window_length, window_width, wall_thickness + 1]);
    }
}

/* ============================ */
/* MODULO: BINARI DI AGGANCIO   */
/* (Sporgenti dal fondo Z=0)    */
/* ============================ */

module launcher_mount() {
    clip_x = (body_length - clip_length) / 2;
    clip_y_left = (body_width - rail_width) / 2 - clip_thickness;
    clip_y_right = (body_width + rail_width) / 2;
    
    difference() {
        union() {
            // Binario Sinistro
            translate([clip_x, clip_y_left, -rail_height]) {
                cube([clip_length, clip_thickness, rail_height + 0.1]);
                cube([clip_length, clip_thickness + rail_lip_depth, 2]); 
            }
            // Binario Destro
            translate([clip_x, clip_y_right, -rail_height]) {
                cube([clip_length, clip_thickness, rail_height + 0.1]);
                translate([0, -rail_lip_depth, 0])
                    cube([clip_length, clip_thickness + rail_lip_depth, 2]); 
            }
            // Muro per far poggiare la molla
            translate([clip_x + clip_length - 8, clip_y_left + clip_thickness, -rail_height])
                cube([10, 4, rail_height + 0.1]);
        }
        
        // Fessura laterale per alloggiare la levetta (asse Z)
        translate([clip_x + clip_length - lock_len + 4, clip_y_left - lock_depth - 1, -rail_height + 1])
            cube([lock_len + 4, lock_depth + clip_thickness + 2, lock_w + 1]);
            
        // Foro verticale per il perno (dal basso verso l'alto)
        translate([clip_x + clip_length - lock_len/2 + 4, clip_y_left, -rail_height - 1])
            cylinder(h=rail_height + 5, d=pivot_hole_dia);
    }
}

module mechanical_lock_clip() {
    difference() {
        union() {
            // Corpo leva
            cube([lock_len, lock_depth, lock_w]);
            
            // Fulcro
            translate([lock_len/2, lock_depth - 0.1, 0])
                cylinder(h=lock_w, d=lock_depth + 2);
                
            // Dente
            translate([-2, lock_depth - 0.1, 0])
                cube([2.1, rail_lip_depth + clip_thickness + 0.1, lock_w]);
                
            // Piolino per molla penna (d=4mm esterno, foro 3mm)
            translate([lock_len - 4, lock_depth - 0.1, lock_w/2])
                rotate([-90, 0, 0])
                    cylinder(h=3.1, d=2.8);
        }
        // Foro perno (asse Z)
        translate([lock_len/2, lock_depth, -1])
            cylinder(h=lock_w + 2, d=pivot_hole_dia);
    }
}

module sensor_slot() {
    w_len = sensor_pcb_w + 2*sensor_slot_clearance;
    w_wid = sensor_pcb_d + 2*sensor_slot_clearance;
    h     = sensor_pcb_h + 2*sensor_slot_clearance;

    x = (body_length - w_len)/2;  // Allineato al centro con i binari
    y = (body_width - w_wid)/2;
    z = wall_thickness;           // Sensore sul fondo del corpo (Z=0)

    translate([x, y, z])
        cube([w_len, w_wid, h]);

    // Buco per permettere ai LED IR di guardare giu
    translate([x + w_len/2, body_width/2, -1])
        cylinder(h=wall_thickness+2, d=sensor_sight_dia);
}

module electronics_bay() {
    // Vano a "Cassetto" dal retro: i componenti si infilano in fila indiana (X)
    cavity_len = mcu_d + batt_d + 3*elec_margin; 
    cavity_w   = max(mcu_w, batt_w) + 2;         
    cavity_h   = max(mcu_h, batt_h) + elec_margin; 
    
    x_pos = body_length - cavity_len;
    y_pos = (body_width - cavity_w) / 2;
    z_pos = (body_height - cavity_h) / 2; 
    
    // Taglio del vano (sfonda il muro posteriore)
    translate([x_pos, y_pos, z_pos])
        cube([cavity_len + 1, cavity_w, cavity_h]);
}

module button_hole() {
    button_x = window_inset / 2;
    translate([button_x, body_width/2, body_height - wall_thickness - 1])
        cylinder(h = wall_thickness + 2, d = button_hole_dia);

    translate([button_x - button_switch_w/2, body_width/2 - button_switch_w/2, body_height - wall_thickness - button_cavity_depth])
        cube([button_switch_w, button_switch_w, button_cavity_depth]);
}

module oled_display_bay() {
    oled_bay_w = oled_pcb_w + 2*oled_clearance;
    oled_bay_d = oled_pcb_d + 2*oled_clearance;
    oled_bay_h = oled_pcb_h + oled_clearance;

    oled_x = oled_x_offset;
    oled_y = (body_width - oled_bay_d) / 2;
    oled_z = body_height - wall_thickness - oled_bay_h;

    translate([oled_x, oled_y, oled_z])
        cube([oled_bay_w, oled_bay_d, oled_bay_h + wall_thickness + 1]);

    window_x = oled_x + (oled_bay_w - oled_window_w) / 2;
    window_y = (body_width - oled_window_h) / 2;
    translate([window_x, window_y, body_height - wall_thickness - 0.5])
        cube([oled_window_w, oled_window_h, wall_thickness + 1]);

    // Foro passante verticale per scendere nel vano elettronica
    translate([oled_x + oled_bay_w/2, body_width/2, oled_z - 10])
        cylinder(h=15, d=8);
}

module display_cover_cap() {
    cap_w = oled_window_w + 2*oled_cap_lip - 0.4;
    cap_h = oled_window_h + 2*oled_cap_lip - 0.4;
    translate([0, 0, 0]) cube([cap_w, cap_h, wall_thickness]);
    
    lip_w = oled_window_w - 0.6;
    lip_h = oled_window_h - 0.6;
    translate([(cap_w - lip_w)/2, (cap_h - lip_h)/2, -1.2])
        cube([lip_w, lip_h, 1.2]);
}

module electronics_lid() {
    // Tappo per chiudere il cassetto posteriore
    cavity_w   = max(mcu_w, batt_w) + 2;
    cavity_h   = max(mcu_h, batt_h) + elec_margin;
    
    lid_w = cavity_w - 2*lid_clearance;
    lid_h = cavity_h - 2*lid_clearance;
    lid_t = 2.4; 
    
    difference() {
        union() {
            cube([lid_w, lid_t, lid_h]);
            // Dentini incastro
            translate([2, 2, 2]) cube([2, 2, 2]);
            translate([lid_w - 4, 2, 2]) cube([2, 2, 2]);
            translate([2, 2, lid_h - 4]) cube([2, 2, 2]);
            translate([lid_w - 4, 2, lid_h - 4]) cube([2, 2, 2]);
            
            // Cornice interna per incastrare il corpo dello Slide Switch (8.6 x 4.2 x 4.2)
            translate([4 - 6.2/2, lid_t, (lid_h - 10.6)/2])
                difference() {
                    cube([6.2, 4.2, 10.6]); 
                    translate([1, -0.1, 1])   
                        cube([4.2, 4.4, 8.6]);
                }
        }
        // Buco USB centrale
        translate([(lid_w - usb_slot_w)/2, -1, (lid_h - usb_slot_h)/2])
            cube([usb_slot_w, lid_t + 2, usb_slot_h]);
            
        // Foro scorrevole per la levetta dello switch
        translate([4 - 1.25, -1, (lid_h - 5)/2])
            cube([2.5, lid_t + 2, 5]);
    }
}

/* ============================ */
/*   ASSIEME CORPO COMPLETO     */
/* ============================ */

module holster_body_final() {
    difference() {
        union() {
            main_shell();
            // launcher_mount(); // RIMOSSO: stampato a parte
            
            // Lip per tappo cieco
            oled_bay_w_lip = oled_pcb_w + 2*oled_clearance;
            oled_bay_h_lip = oled_pcb_h + oled_clearance;
            window_x_lip = oled_x_offset + (oled_bay_w_lip - oled_window_w) / 2;
            window_y_lip = (body_width - oled_window_h) / 2;
            translate([window_x_lip - oled_cap_lip, window_y_lip - oled_cap_lip, body_height - wall_thickness - 1.5])
                cube([oled_window_w + 2*oled_cap_lip, oled_window_h + 2*oled_cap_lip, 1.5]);
        }
        sensor_slot();
        button_hole();
        electronics_bay();
        oled_display_bay();
    }
}

/* ============================ */
/*         RENDER FINALE        */
/* ============================ */

holster_body_final();

// Sportello elettronica (distanziato Y=50)
translate([0, 50, 0])
    electronics_lid();

// Tappo cieco display (distanziato Y=70)
translate([0, 70, 0])
    display_cover_cap();

// La levetta di blocco e la slitta sono nel file test_hook_only.scad
