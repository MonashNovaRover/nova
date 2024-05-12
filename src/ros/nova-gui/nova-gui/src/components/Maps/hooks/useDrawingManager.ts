import { useMap, useMapsLibrary } from "@vis.gl/react-google-maps";
import { useState, useEffect } from "react";

export function useDrawingManager(
  initialValue: google.maps.drawing.DrawingManager | null = null
) {
  const map = useMap();
  const drawing = useMapsLibrary("drawing");

  const [drawingManager, setDrawingManager] =
    useState<google.maps.drawing.DrawingManager | null>(initialValue);

  useEffect(() => {
    if (!map || !drawing) return;

    // https://developers.google.com/maps/documentation/javascript/reference/drawing
    const newDrawingManager = new drawing.DrawingManager({
      map,
      drawingMode: null,
      drawingControl: false,
      drawingControlOptions: {
        drawingModes: [google.maps.drawing.OverlayType.MARKER],
      },
      markerOptions: {},
    });
    setDrawingManager(newDrawingManager);
    return () => {
      newDrawingManager.setMap(null);
    };
  }, [drawing, map]);

  return drawingManager;
}
