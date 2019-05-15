include <gear.scad>            
include <ruler.scad>

n1           = 17; // number of teeth
mm_per_tooth = 5; 
thickness    = 5;
hole         = 0;

// translate([ 0, -15, 3]) ruler(200);

union(){
    difference(){
        color([1.00,1.00,1.00])
        gear(mm_per_tooth,n1,thickness,hole);

        color([1.00,1.00,1.00])
        cylinder(thickness + 1, 4, 4, center=true);
        
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
}
