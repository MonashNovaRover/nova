# auto_object_localisation
This folder contains the ROS2 packages responsible for object localisation for the autonomous stack.

Documentation can be found in 3 places:
- Notion: https://www.notion.so/Object-Detection-19db713961718033b820fc406df6697f
- Readmes in the subfolders
- Code Comments

```
Apologies for the messy comments,
I was mainly responsible for object detection and localisation in the 2024-2025 cycle and it never got used in either of the competitions that year.
So if you are a recruit reading this, please please please get this stuff used in real life!
It should all work, but probably only 50% well so improve refine and maybe write it in cpp lol.
- Anthony Lew :D
```

Notes for improvement
- Object localisation algorithm sucks!
-   See `./nova_object_localisation/nova_object_localisation/object_localiser.py`
- Object localisation needs better outlier removal
- Object localisation could have an ARCh/URC mode switcher
- Object detection overlay cannot handle different types of detections and is generally not robust enough.
-   See `nova_detection_overlay/nova_detection_overlay/detection_overlay.py`
