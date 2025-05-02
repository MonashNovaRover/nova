import { useEffect } from "react";
import { useSelector } from "react-redux";
import { useBifrost } from "../../redux/actions/bifrost/useBifrostAction";
import { useCartographerActions } from "../../redux/actions/useCartographerActions";
import { RootState } from "../../redux/RootState";
import { RosTopic } from "../../ros/topics/rosTopic";
import { MapTilerMap } from "./maptiler/MapTilerMap";
import { ToolTipButton } from "../shared/TooltipButton";
import { PropRenderer } from "../shared/PropRenderer";
import { MapInteractionMode, MapPoint } from "../../redux/models/CartographerState";
import { MapPin, X } from "react-feather";
import { NewMarkerModal } from "./components/NewMarkerModal";
import { BottomOverlay } from "./components/BottomOverlay";
import { Rulers } from "react-bootstrap-icons";
import { getDistance } from "./utils/geojson";
import { useLocalStorage } from "../../hooks/useLocalStorage.ts";
import { MapTile } from "./config";

export const Cartographer = () => {
  const [mapTile, setMapTile] = useLocalStorage("mapTile", MapTile.Hanksville);
  const [storedPoints, setStoredPoints] = useLocalStorage("storedPoints", [] as MapPoint[])

  const roverLocationBifrost = useBifrost({
    topic: RosTopic.ROVER_LOCATION,
  });

  const baseLocationBifrost = useBifrost({
    topic: RosTopic.BASE_LOCATION,
  });

  const { mapInteractionMode, mousePosition, newMarkerModal, measure } =
    useSelector((state: RootState) => state.cartographerState);

  useEffect(() => {
    roverLocationBifrost.syncWithTopic();
  }, [roverLocationBifrost]);

  useEffect(() => {
    baseLocationBifrost.syncWithTopic();
  }, [baseLocationBifrost]);

  const { setPoints, setInteractionMode, closeNewModal, clearMeasurements } =
    useCartographerActions();

  const addPoint = (point: MapPoint) => {
    setStoredPoints([...storedPoints, point])
  }

  const deletePoint = (point: MapPoint) => {
    setStoredPoints([...storedPoints.filter(p => p.name !== point.name)])
  }
  
  useEffect(() => {
    setPoints([...storedPoints])
    // eslint-disable-next-line react-hooks/exhaustive-deps
  },[storedPoints])

  return (
    <div className="w-full ">
      <NewMarkerModal
        isOpen={newMarkerModal.open}
        addPoint={addPoint}
        closeModal={closeNewModal}
        latitude={newMarkerModal.coordinate?.lat}
        longitude={newMarkerModal.coordinate?.long}
      />
      <div className="flex h-[95vh]">
        <MapTilerMap
          mapTile={mapTile}
          overlay={
            <>
              <div className="flex flex-col justify-end gap-2 absolute top-2 right-2 mr-12">
                <div className="flex flex-row justify-end gap-1">
                  {mapInteractionMode === MapInteractionMode.SELECT &&
                    mousePosition && (
                      <PropRenderer
                        props={{
                          latitude: mousePosition.lat,
                          longitude: mousePosition.long,
                        }}
                        ignoreProps={[]}
                        row
                        size="sm"
                      />
                    )}

                  <ToolTipButton
                    placement="left"
                    tooltipContent={"Drop Pins"}
                    className="bottom-0 right-0"
                    variant="shadow"
                    isIconOnly
                    size="lg"
                    color={
                      mapInteractionMode === MapInteractionMode.SELECT
                        ? "success"
                        : "default"
                    }
                    onClick={() =>
                      setInteractionMode(
                        mapInteractionMode === MapInteractionMode.SELECT
                          ? MapInteractionMode.PAN
                          : MapInteractionMode.SELECT
                      )
                    }
                  >
                    <MapPin size={20} />
                  </ToolTipButton>
                </div>
                <div className="flex flex-row justify-end gap-1">
                  {mapInteractionMode === MapInteractionMode.MEASURE &&
                    mousePosition && (
                      <PropRenderer
                        props={{
                          distance:
                            measure.from && measure.to
                              ? getDistance(
                                  measure.from,
                                  measure.to
                                ).toString() + "m"
                              : "0m",
                        }}
                        ignoreProps={[]}
                        row
                        size="sm"
                      />
                    )}

                  <ToolTipButton
                    placement="left"
                    tooltipContent={
                      measure.from && measure.to && !measure.measuring
                        ? "Clear Measurement"
                        : "Measure"
                    }
                    className="bottom-0 right-0"
                    variant="shadow"
                    isIconOnly
                    size="lg"
                    color={
                      mapInteractionMode === MapInteractionMode.MEASURE
                        ? "success"
                        : "default"
                    }
                    onClick={() => {
                      if (mapInteractionMode === MapInteractionMode.MEASURE) {
                        clearMeasurements();
                        setInteractionMode(MapInteractionMode.PAN);
                      } else {
                        setInteractionMode(MapInteractionMode.MEASURE);
                      }
                    }}
                  >
                    {measure.from && measure.to && !measure.measuring ? (
                      <X size={20} />
                    ) : (
                      <Rulers size={20} />
                    )}
                  </ToolTipButton>
                </div>
              </div>
            </>
          }
        />
        <div className="fixed bottom-0 w-full">
          <BottomOverlay mapTile={mapTile} setMapTile={setMapTile as (tile: MapTile) => void} deletePoint={deletePoint}/>
        </div>
      </div>
    </div>
  );
};
