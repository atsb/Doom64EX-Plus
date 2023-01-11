# Using DLight

    Before building lightmaps, the wad file must first be pre-compiled with
    GL nodes using GLBSP (http://glbsp.sourceforge.net/). Optionally, PVS
    data can be built using GLVIS (http://www.vavoom-engine.com/glvis.php)
    which can help speed up compile time.
    
    The input for DLight is as follows:
    
    dlight -map ## [option1] [option2] ... [wad file]
    
    As of version 2.0, DLight can only compile one map at a time so the
    -map option is required.
    
# Options
    
    -map <1-99>             Specify the map number to compile for
    
    -samples <2, 4, 8, 16>  Specify how many samples to build. Samples are
                            automatically rounded into powers of two. Higher
                            sample count will result in faster compile times
                            but results in poor quality lightmaps, while
                            lower values will generate better quality
                            lightmaps but results in longer compile times.
                            Using 8 or 16 is recommended.
                            
    -size <64, 128, 256>    Specify the width and height of the lightmap
                            textures. Size is automatically rounded into
                            powers of two. Using 128 or 256 is recommended.
                            
    -threads <##>           Specify how many threads to utilize for building
                            lightmaps. By default, the tool will try to
                            utilize 2 threads.
                            
    -help                   Displays list of options
    
    -config <file path>     Specify a configuration file to use. By default
                            it will try to find SVE.CFG
                            
    -writetga               Dumps generated lightmaps as targa image files
    
    -ambience <##>          UNUSED
    
# DLight Configuration File Specification

    Format:
            /*
                multi-line comment
            */
            
            // single line comment
            
            block name
            {
                <property>  <value>
            }
            
            integer         a single whole number
            float           a number with decimals
            vector          a quoted string with 3 floating point or
                            integer numbers usually in the form of xyz
            boolean         either 0 or 1
                     
            
    Blocks:
            mapdef
            lightdef
            surfaceLight


    Mapdef block:
        A mapdef defines the global sunlight behavior for a specified map.
        
    Mapdef properties:
        map <integer>               The map number this mapdef block should
                                    be assigned to
                            
        sun_direction <vector>      A vector representing the direction in
                                    which the direction of the sun is coming
                                    from. Vector is normalized, meaning each
                                    component ranges from 0.0 to 1.0 and is
                                    measured in Doom axis coordinates
                                    (e.g. x = left, y = forward, z = up)
                                    Default vector: "0.45 0.3 0.9"
                                    
        sun_color <vector>          A vector representing the RGB color of
                                    the sunlight. Values ranges from 0 to
                                    255 Default vector: "255 255 255"
                                    
        sun_ignore_tag <integer>    If specified, any sector with this tag
                                    will not be lit by sunlight and no
                                    lightmaps will be generated for it.
                                    
    Lightdef block:
        A lightdef block allows to define a thing to emit light during
        compile time.
        
    Lightdef properties:
    
        doomednum <integer>         The doomednum of this thing in which
                                    will generate lightmaps.
                                    
        rgb <vector>                A vector representing the RGB color of
                                    the light
        
        height <float>              Sets an offset on the z-axis for the
                                    origin of the light source.
                                    
        radius <float>              Specifies the radius of the light. A
                                    larger radius will result in more
                                    surfaces affected by this light.
                                    Setting radius to -1 will tell DLight
                                    to take the radius based on the
                                    thing's angle value.
                                    
        intensity <float>           Specifies how strong the light source
                                    is. A value of 4.0 is recommended in
                                    Strife: Veteran Edition
                                    
        falloff <float>             Sets the falloff for this light
                                    (expiremental)
        
        ceiling                     If specified, then the origin of the
                                    light will be relative to the ceiling
                                    instead of the floor
                                    
    SurfaceLight block:
        A surfacelight block specifies a surface (either a wall or a flat) to
        emit light during compile time.
        
    SurfaceLight properties:
    
        tag <integer>               Tells Dlight to emit light from all
                                    sectors/lines that contains this tag value.
                                    
        rgb <vector>                A vector representing the RGB color of
                                    the light
        
        cone_inner <float>          Specifies the FOV (in degrees) of the main
                                    source of light. Value ranges between 0 to
                                    180 degrees. Smaller values results in
                                    larger cone.
                                    
        cone_outer <float>          Specifies the FOV (in degrees) of the light
                                    fading out from the inner cone.
                                    
        distance <float>            Specifies how much area to cover
                                    
        intensity <float>           Specifies how strong the light source is.
                                    A value of 4.0 is recommended in
                                    Strife: Veteran Edition
                                    
        bIgnoreFloor <boolean>      If 1, then floors will be ignored.
                                    
        falloff <float>             Sets the falloff for this light
                                    (expiremental)
    
# Map Lump Specifications

    LM_MAP##                        Level marker
    
    LM_CELLS                        Contains rgb information on how sprites
                                    interacts with lightmaps.
                                    
    LM_SUN                          Contains RGB and vector data for sunlight.
                                    
    LM_SURFS                        Contains information for every surface
                                    that is bounded to a lightmap. This
                                    includes texture mapping (UV) data and
                                    which lightmap block the surface is
                                    mapped to.
                                    
    LM_TXCRD                        Contains all UV texture coordinates for
                                    each surface.
                                    
    LM_LMAPS                        Contains raw RGB texture data that makes
                                    up the lightmaps.
