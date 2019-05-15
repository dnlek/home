include <gear.scad>            
include <ruler.scad>

n1           = 17; // number of teeth
mm_per_tooth = 5; 
thickness    = 5;
hole         = 0;

// translate([ 0, 0, 3]) ruler(200);

union(){
    difference(){
        color([1.00,1.00,1.00])
        // gear(mm_per_tooth,n1,thickness,hole);
        cylinder(thickness, 17, 17, center=true);
        
        color([1.00,1.00,1.00])
        cube(size=[8, 8, thickness + 1], center=true);
        
        color([1.00,1.00,1.00])
        translate([0, 0, -2])
        cylinder(thickness / 3, 6, 6, center=true);

        color([1.00,1.00,1.00])
        translate([8, 0, -1 * thickness +2/ 2])
        cylinder(thickness + 1, 1, 1);
        
        color([1.00,1.00,1.00])
        translate([0, 8, -1 * thickness +2/ 2])
        cylinder(thickness + 1, 1, 1);
        
        color([1.00,1.00,1.00])
        translate([-8, 0, -1 * thickness + 2/ 2])
        cylinder(thickness + 1, 1, 1);
        
        color([1.00,1.00,1.00])
        translate([0, -8, -1 * thickness + 2 / 2])
        cylinder(thickness + 1, 1, 1);
    }
    
    difference() {
        color([1.00,1.00,1.00])
        translate([-4, -1, -1])
        cube(size=[8, 2, thickness - 2]);
        
        color([1.00,1.00,1.00])
        cube(size=[4, 4, thickness + 1], center=true);
    }
}
