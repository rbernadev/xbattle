// =====================================================================
// BEYBLADE X - TEST AGGANCIO BATTLE PASS + CLIP A MOLLA (BILANCIERE Z)
// =====================================================================

$fn = 48;

// --- PARAMETRI DIMENSIONALI SLITTA ---
rail_width           = 22.0;
rail_height          = 8.0;
rail_lip_depth       = 2.5;
clip_thickness       = 3.0;
clip_length          = 40.0;
sensor_sight_dia     = 6.0;

// --- PARAMETRI CLIP MECCANICA (Bilanciere Z) ---
pivot_hole_dia       = 2.0;   // Foro per filamento 1.75mm
lock_w               = 6.0;   // Altezza (Z) della levetta
lock_len             = 24.0;  // Lunghezza totale (X)
lock_depth           = 4.0;   // Profondita (Y)

// Quote di base
body_length          = 130;
body_width           = 32;
clip_x               = (body_length - clip_length) / 2;

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
        // Foro perno
        translate([lock_len/2, lock_depth, -1])
            cylinder(h=lock_w + 2, d=pivot_hole_dia);
    }
}

module launcher_mount() {
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
        
        // Fessura laterale per alloggiare la levetta
        translate([clip_x + clip_length - lock_len + 4, clip_y_left - lock_depth - 1, -rail_height + 1])
            cube([lock_len + 4, lock_depth + clip_thickness + 2, lock_w + 1]);
            
        // Foro verticale per il perno (da sotto fino al corpo)
        translate([clip_x + clip_length - lock_len/2 + 4, clip_y_left, -rail_height - 1])
            cylinder(h=rail_height + 5, d=pivot_hole_dia);
    }
}

module test_hook_block() {
    difference() {
        translate([(body_length - clip_length)/2 - 5, 0, 0])
            cube([clip_length + 15, body_width, 2.4]);
        translate([(body_length)/2, body_width/2, -1])
            cylinder(h=5, d=sensor_sight_dia);
    }
    launcher_mount();
}

translate([-(body_length)/2, -(body_width)/2, 0])
    test_hook_block();

translate([0, 30, 0])
    mechanical_lock_clip();
