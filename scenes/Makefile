all : \
	../dist/hexapod.pnct \
	../dist/hexapod.scene \


../dist/hexapod.scene : hexapod.blend export-scene.py
	"E:\Program Files\Blender Foundation\Blender 2.90\blender.exe" --background --python export-scene.py -- "hexapod.blend:Main" "../dist/hexapod.scene"

../dist/hexapod.pnct : hexapod.blend export-meshes.py
	"E:\Program Files\Blender Foundation\Blender 2.90\blender.exe" --background --python export-meshes.py -- "hexapod.blend:Main" "../dist/hexapod.pnct"
