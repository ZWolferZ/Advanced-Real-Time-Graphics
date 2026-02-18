### **Camera Controls:**



W - Move Camera Forward.



A - Move Camera Left.



S - Move Camera Backwards.



D - Move Camera Right.



Hold Right Click - Activate Camera Pivot Mode.



Unpressed Right Click - Deactivate Camera Pivot Mode.



Mouse Look - Rotate Viewport (CAMERA PIVOT MODE ON).



Left Click Object - Pick/Select/Play Object (DEFERRED LIGHTING MODE ON)



### **Documentation:**



###### **Renderer Features:**



Normal Mapping Via The UI Window - "Normal Mapping Selection Window".



Post Processing Effects Via The UI Window - "Post Processing Window".



3D Camera Movement Via WASD and Mouse Look.



Position / Rotation / Scale Gizmos For Lights and Objects.



Camera / Object Picking Via Sampling The World Position Buffer.



Full Deferred Lighting Implementation, Deferred Texture Stages Visible In "Deferred Lighting Window".



Render To Texture Scene Objects.



3 Separate Demo Scenes - Accessible Through The "Teleportation Window".



Audio Effects - Via miniaudio.h / miniaudio.c.



Object Scene Dialog Bubbles - Playing Tutorial Help.



Window Resizing.



###### **ImGui Windows:** 



* Windows are locked in place to prevent any weirdness.



* Most Windows have components that can be dragged, clicked or scrolled upon.



* Some Windows only appear under certain conditions - object has been selected etc.



* Deferred Lighting Mode can be toggled on or off for showcasing purposes, some features NEED this mode toggled on and visa versa.



* Windows Render Outside of the Main Viewport when not in Fullscreen mode, *You cannot escape them...*



* Use the Hide All Windows button if the UI becomes too much, the renderer starts with this toggled on.



**Window List:**



* RyanLabs DX11 Renderer Window - Showcasing App Stats.
* Texture Selection Window - List of Loaded Textures To Apply To Selected Object. (NEEDS SELECTED OBJECT TO APPEAR)
* Normal Map Selection Window - List of Loaded Normal Maps To Apply To Selected Object. (NEEDS SELECTED OBJECT TO APPEAR)
* Post Processing Window - List of Options to change the final screen output.
* Camera Statistics Window - Options for the camera.
* Camera Spline Animation Window - Custom Camera Spline Editor. (NEEDS "Show Camera Spline Window" ON TO APPEAR)
* Deferred Lighting Textures Window - Option to Toggle between deferred lighting and forward rendering, Also showcases different renderpasses.
* Light Selection Window - List of Lights to Choose from, Unlike objects this is the only way to select lights to move around in the scene.
* Light Properties Update Window - Options to Move and change lights. (NEEDS SELECTED LIGHT TO APPEAR)
* Object Selection Window - List of Objects to Choose from, Objects can also be selected via mouse click / camera picking.
* Object Gizmo Type Window - Different Gizmo Types to select from, also available from the hot binds "1,2,3". (NEEDS SELECTED OBJECT TO APPEAR)
* Object Movement Window - List of Options to Move a object. (NEEDS SELECTED OBJECT TO APPEAR)
* Object Material Buffer - List of Options to Change a objects appearance.(NEEDS SELECTED OBJECT TO APPEAR)
* Object Mesh/Model Selection Window - List of Loaded Models to Change a objects mesh. (NEEDS SELECTED OBJECT TO APPEAR)



###### **Random Freezes:**



Some random freezes can occur on start-up when building the project from Source in Visual Studio, if this occurs, just close then run the project again. 



A built executable has also been included in the root folder if the project wont build from Source.



###### **Important Notes:**



Please run the project in release mode if building from Visual Studio.



The Tutorial Dialog objects are only visible when Show/Hide windows is toggled OFF and when Deferred Lighting is Toggled ON.



Render Textures of the Deferred Lighting Stages will disappear when deferred lighting is toggled OFF and the Window size is changed, this is expected behaviour.



**Have fun David!**







